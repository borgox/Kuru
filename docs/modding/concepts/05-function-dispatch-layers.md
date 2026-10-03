# Function dispatch layers

A single logical call - "run this function on this object" - can reach the engine through several different
C++ entry points. Each is a *dispatch layer*. Kuru hooks these layers, and understanding which one a call
passes through, and which one holds the parameters, is the core of the hook system.

## The layers

These are methods on `UObject` (found by vtable index in the Rocket League fork), plus one non-virtual
function:

| Layer | Found by | Role | Parameters |
| --- | --- | --- | --- |
| `ProcessEvent` | vtable 67 | the main entry to call a `UFunction` on an object | flat `Parms` blob |
| `ProcessInternal` | non-virtual | runs a *script* function's body (the interpreter) | frame `Locals` |
| `CallFunction` | vtable 76 | bytecode call dispatch - where script *calls* a function | inline in bytecode |

Kuru hooks only these three. The other delegate/event entry points all funnel into them, so hooking them
separately would only cause duplicate fires: `ProcessDelegate` (vtable 68), `CallDelegate` (vtable 69) and
`ProcessRemoteFunction` (vtable 71) are each always reached *through*
`ProcessEvent`/`ProcessInternal`/`CallFunction`, and `ProcessState` (vtable 70) isn't safely hookable, so
Kuru leaves them all alone.

## The same call, several layers

The layers aren't alternatives - one call can pass through more than one. When script calls a normal script
function, it reaches `CallFunction`, which sets up a frame and hands off to `ProcessInternal` to run the body.
When C++ raises an event, it goes through `ProcessEvent`, which for a script function also ends at
`ProcessInternal`.

So if Kuru naively hooked every layer, one call could fire your hook two or three times - and at some of those
layers the parameters wouldn't exist yet (a native at `CallFunction` hasn't materialised its args; see
[the VM](04-the-unrealscript-vm.md)).

## The canonical layer

Kuru solves this by choosing exactly **one** layer per function where a name-based hook fires - the layer that
actually has the parameters. It's decided from the function's flags, with no guessing:

- **Non-native with a script body** (`FUNC_Defined`) -> `ProcessInternal`. Such functions always funnel there,
  and their parameters are in the frame `Locals`.
- **Everything else** - natives (they read args inline) and body-less stubs - -> `CallFunction`. That's the
  only layer they ever reach.

This is why you write `Hooks::Pre("Function ...")` once and it fires in the right place with usable
parameters, without thinking about layers. (You can still target a specific layer with `PreVia` / `PostVia`
when you need to.)

## Recursion dedup

Some calls re-enter - a function ends up invoking itself again through a different layer. Kuru keeps a small
per-thread stack of the functions currently being dispatched, so a name-based hook fires **once per top-level
call** even under recursion. Only the canonical layer touches that stack, which is what pins the count at
exactly one.

## Why it's built this way

The layers exist because the engine has several reasons to call a function - events, script calls, delegates,
networking - and each grew its own path. Kuru can't remove that; instead it maps the paths, picks the one
useful point per function, and hides the rest. The [hook engine](07-the-hook-engine.md) page is how that
mapping is implemented, and [delegates](09-delegates.md) covers how delegates dispatch.
