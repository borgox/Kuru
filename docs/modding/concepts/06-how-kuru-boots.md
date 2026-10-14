# How Kuru boots

Kuru is a DLL injected into Rocket League. Between "the DLL loaded" and "mods are running" it has to find the
game's internals. Most bindings are resolved at runtime, but fixed core layouts, vtable indices, and an
interpreter-global RVA fallback remain version-specific.

## Injection and the worker thread

The injector loads `KuruLoader.dll` into the running game. `DllMain` does almost nothing itself - doing real
work inside the Windows loader lock is unsafe - so it spawns a worker thread and returns immediately.
Everything below happens on that thread.

## Waiting for the game

The injector can attach before the engine has built its object array, so the worker waits first: it polls
until a globals candidate validates and `GObjects` has data. It retries up to 200 times with a 50 ms sleep
between attempts. Failure is logged and stops that worker. Injecting at the main menu is the normal
workflow; early injection is not guaranteed to succeed.

## Resolving the globals

Bootstrap resolves the engine components in this order, using signatures or runtime data instead of an
address:

- **`GNames` and `GObjects`** - a data signature is scanned across the module image. Its match address
  is used directly as the GNames array header; GObjects is derived at `+0x48`. The validator checks
  both array headers and sampled object/name relationships. No instruction operand is decoded here.
- **`ProcessEvent` / `CallFunction`** - a live object's vtable supplies entries 67 and 76. This reads
  entry-point addresses, not a guarantee that every object's whole vtable is identical.
- **`GMalloc`** - a separate pattern resolves the allocator global slot. The host checks that its table
  and slots 2/3/4 are readable/executable before publishing Malloc/Realloc/Free bindings. If unavailable,
  bootstrap stops to avoid falling back to incompatible allocation.
- **`ProcessInternal`** - scans non-native `UFunction` objects for a non-null `Func`, using the earlier
  finding that a script function's pointer names the interpreter. This is still a build-dependent assumption.
- **`GNatives`** - found by scanning `ProcessInternal`'s code for the table access, then validating the
  candidate's first 256 entries point into the game's executable section. That is a validation heuristic,
  not a uniqueness proof or a statement about the full size of every native-dispatch table.
- **The interpreter globals** (`GProperty`, `GPropAddr`, `GPropObject`, `GRuntimeUCFlags`) - found by scanning
  a `GNatives` handler for its writes to those globals, then confirmed by calling the handler with a known
  object/property and matching the results by value. If runtime resolution fails, the current host
  supplies hardcoded module-relative addresses and logs that fallback. They must not be assumed valid
  for a different game build.

Each step leans on the previous one - `ProcessInternal` yields `GNatives`, `GNatives` yields the interpreter
globals - so one reliable entry point unfolds into everything else.

## Installing hooks and loading mods

Before enabling engine detours, Kuru scans the `mods` folder beside its DLL and loads mods on the
bootstrap worker. It checks required host API versions, supplies runtime/API bindings, creates each
instance, and calls initial `OnLoad`. A false return rolls back host registrations and pending work
before destroying the partial instance and releasing its DLL. Initial factory/OnLoad calls are not
covered by the re-enable/reload C++ exception wrapper.

It then installs ProcessEvent, CallFunction, and ProcessInternal detours where resolved, followed by
Present/ResizeBuffers overlay hooks and the raw mouse capture attempt. Present initializes the ImGui
context; ImGui frames are built through messages to the game-window thread and rendered as snapshots.
Not every hook-install result is fatal to the worker, so inspect the log rather than treating the
status of LoadLibrary as proof of complete initialization.

## What survives an update

Runtime resolution can tolerate relocated code when the instruction patterns and layouts still match.
It does not make the loader update-proof. Patterns, vtable slots, interpreter behavior, manually declared
core layouts, and fallback RVAs can all change. See [updating for a game version](../../maintaining/updating-for-a-new-version.md).

Source: [DllMain.cpp](../../../libs/KuruLoader/src/DllMain.cpp),
[Gui.cpp](../../../libs/KuruLoader/src/Gui.cpp), and
[ThreadBridge.cpp](../../../libs/KuruLoader/src/ThreadBridge.cpp).
