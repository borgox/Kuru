# The hook engine

The [dispatch layers](05-function-dispatch-layers.md) page described *what* Kuru hooks and *which* layer fires
for a given function. This page describes registration, dispatch, and callback lifetime. Mod code still
runs inside the game process; neither these tables nor exception guards guarantee crash safety.

## Detours

At boot, Kuru installs a trampoline hook (via MinHook) on each dispatch layer's entry - the vtable methods and
`ProcessInternal`. From then on, when the game calls one of those, Kuru's detour runs first, does its work,
and calls the original through the trampoline. Only calls reaching these intercepted entry points are covered.

## A pointer-keyed registry

Hooks are registered by function *name*, but names are slow to match on a hot path. So when you register a
hook, Kuru resolves the name to its `UFunction*` **once** and stores the hook keyed by that pointer. The detour
receives the `UFunction*` directly, so "does this call have hooks?" is a single hash lookup with no string
work - and the vast majority of calls, which aren't hooked, skip everything after it.

## Dispatch

When a hooked function fires, the detour:

1. looks up its pre and post hooks by `UFunction*`,
2. decides whether an `Any` (name-based) hook should fire here - true only at the function's canonical layer,
   and only once per call (the recursion guard from the [dispatch layers](05-function-dispatch-layers.md)
   page),
3. runs the matching pre hooks, each with a `HookContext` giving `Caller`, `Function`, the parameters, and
   `Block`,
4. calls the original - unless a pre hook blocked it,
5. runs the post hooks, now with the result filled in.

## Shared snapshots and copy-on-write tables

Detours run on the thread entering the intercepted function. Each loads an
`atomic<shared_ptr<const HookTables>>` snapshot and retains it across pre/original/post dispatch.
Writers serialize under a mutex, clone the tables, change the clone, and publish it atomically.
Old tables are reclaimed after their readers release them. Atomic shared-pointer operations may use
internal synchronization; dispatch is not guaranteed lock-free.

API v4 entries share callback ownership across snapshots. Removal marks an entry so stale snapshots
skip pending invocations; already running callbacks keep their storage until completion. Captured
destructors are released outside registry writer locks. Mod teardown removes its entries before DLL unload.

## Fault isolation

A mod callback runs arbitrary code, and a buggy one can dereference bad memory. That's an access violation - a
*structured* exception, which a normal C++ `try/catch` does not catch. Without protection, one bad mod would
crash the whole game. So Kuru calls each callback inside a structured-exception guard: a fault is caught,
logged with the function that triggered it, and dispatch can continue. This cannot undo memory
corruption, guarantee C++ stack unwinding, or cover faults elsewhere in the engine. Hook faults are
logged but not automatically removed; faulting module-event listeners are removed.

## The event bus on top

The [event bus](../guides/events.md) is a thin layer over this engine. A `Pre`/`PostFunctionCallEvent` listener
is registered as an ordinary hook on the named function - it reuses everything here, canonical routing and
dedup included. Module-defined events don't touch the game at all; they go through a separate listener registry
that fans out synchronously on the dispatching thread. It copies matching listeners under a mutex,
then invokes them outside it. Higher priorities run first. This supports nested dispatch without
holding that registry mutex while calling mod code.

Source: [HookEngine.cpp](../../../libs/KuruLoader/src/HookEngine.cpp) and
[CallbackLifetime.hpp](../../../libs/KuruLoader/src/CallbackLifetime.hpp).
