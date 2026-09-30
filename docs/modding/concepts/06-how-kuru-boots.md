# How Kuru boots

Kuru is a DLL injected into Rocket League. Between "the DLL loaded" and "mods are running" it has to find the
game's internals - and it does so without any hardcoded addresses, which is what lets it survive game updates.

## Injection and the worker thread

The injector loads `KuruLoader.dll` into the running game. `DllMain` does almost nothing itself - doing real
work inside the Windows loader lock is unsafe - so it spawns a worker thread and returns immediately.
Everything below happens on that thread.

## Waiting for the game

The injector can attach before the engine has built its object array, so the worker waits first: it polls
until `GObjects` exists and actually has objects in it. Only then does the rest of bootstrap run. This is why
you can inject early, or auto-inject on launch, without the loader silently failing.

## Resolving the globals

Bootstrap finds each piece of the engine it needs, in order - each grounded in something stable rather than an
address:

- **`GNames` and `GObjects`** - found by scanning the game's code for a byte pattern that references them. The
  pattern is a distinctive instruction sequence; its operand gives the address.
- **`ProcessEvent` and the other vtable layers** - read from a live object's vtable at their known indices
  (67, 68, 71, 76, ...). Every object shares that vtable, so the first valid object works.
- **`ProcessInternal`** - every non-native `UFunction` has its `Func` pointer set to `ProcessInternal`, so
  Kuru reads it straight off a script function. Nothing to pattern-match.
- **`GNatives`** - found by scanning `ProcessInternal`'s code for the table access, then validating the
  candidate is 256 code pointers (nothing else in the module looks like that).
- **The interpreter globals** (`GProperty`, `GPropAddr`, `GPropObject`, `GRuntimeUCFlags`) - found by scanning
  a `GNatives` handler for its writes to those globals, then confirmed by calling the handler with a known
  object and matching the results by value.

Each step leans on the previous one - `ProcessInternal` yields `GNatives`, `GNatives` yields the interpreter
globals - so one reliable entry point unfolds into everything else.

## Installing hooks and loading mods

With the layers resolved, Kuru installs its detours (via MinHook) on the vtable entries and `ProcessInternal`,
then scans the `mods/` folder and loads each mod: it pushes the resolved game bindings and the host API into
the mod, calls its `OnLoad`, and registers whatever hooks and commands it asks for.

## Why it's update-proof

None of this is a fixed address. Vtable indices, byte patterns, and structural facts - a table of 256 code
pointers, `Func` equal to `ProcessInternal` - all survive the game moving code around between updates. The
parts that *can* change are struct layouts (rare) and the occasional byte pattern (when Psyonix rewrites the
code around a global). Keeping those working is the subject of the [maintaining](../../maintaining) docs.
