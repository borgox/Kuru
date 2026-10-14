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

Kuru currently hooks only these three. It does not separately detour ProcessDelegate, CallDelegate,
ProcessRemoteFunction, or ProcessState. Earlier research assigns additional vtable slots to these
entry points, but their absence from the hook engine is not proof that every delegate or network
path always passes through an intercepted layer. Direct native C++ calls can bypass reflection dispatch.

## The same call, several layers

The layers aren't alternatives - one call can pass through more than one. When script calls a normal script
function, it reaches `CallFunction`, which sets up a frame and hands off to `ProcessInternal` to run the body.
When C++ raises an event, it goes through `ProcessEvent`, which for a script function also ends at
`ProcessInternal`.

So if Kuru naively hooked every layer, one call could fire your hook two or three times - and at some of those
layers the parameters wouldn't exist yet (a native at `CallFunction` hasn't materialised its args; see
[the VM](04-the-unrealscript-vm.md)).

## The canonical layer

Kuru chooses one canonical layer for `Any` hooks from the function's flags:

- **Non-native with a script body** (`FUNC_Defined`) -> `ProcessInternal`, where the interpreter's parameters
  are in `Locals`.
- **Everything else** -> `CallFunction`. The native test is `iNative != 0` or `FUNC_Native` set.

This is Kuru's routing policy (`CanonicalLayer`), not an exhaustive description of possible engine calls.
In particular, a native invoked only through ProcessEvent does not reach the canonical CallFunction
layer, so an `Any` hook will not fire there. Use an explicit ProcessEvent hook when that is the path
you intend to observe. Explicit layers can also produce multiple callbacks if a call crosses them.

`Hooks::Pre("Function ...")` uses this automatic policy. `PreVia` / `PostVia` select an explicit layer when needed; they
do not guarantee parameter reconstruction at that layer.

## Recursion dedup

Some calls re-enter - a function ends up invoking itself again through a different layer. Kuru keeps a small
per-thread stack of the functions currently being dispatched, so a name-based hook fires **once per top-level
call** even under recursion. Only the canonical layer touches that stack, which is what pins the count at
one. The key is only `UFunction*`, not caller object: a nested call of the function on another object
on the same thread is suppressed too. Explicit-layer hooks (`PreInternal`, `PreVia`/`PostVia`) bypass
that `Any` policy.

## Why it's built this way

The layers exist because the engine has several reasons to call a function - events, script calls, delegates,
networking - and each grew its own path. Kuru can't remove that; instead it maps the paths, picks the one
useful point per function, and hides the rest. The [hook engine](07-the-hook-engine.md) page is how that
mapping is implemented, and [delegates](09-delegates.md) covers how delegates dispatch.
