# The hook engine

The [dispatch layers](05-function-dispatch-layers.md) page described *what* Kuru hooks and *which* layer fires
for a given function. This page is *how* - the machinery that makes a name-based hook fast, thread-safe, and
unable to crash the game.

## Detours

At boot, Kuru installs a trampoline hook (via MinHook) on each dispatch layer's entry - the vtable methods and
`ProcessInternal`. From then on, when the game calls one of those, Kuru's detour runs first, does its work,
and calls the original through the trampoline. The detour is the single choke point every call flows through.

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

## Lock-free dispatch, copy-on-write tables

Dispatch happens on the game thread, constantly, and must not take a lock on every call. So the hook tables are
copy-on-write: the detour loads the current table snapshot with a single atomic read and uses it. A writer
(registering or removing a hook) clones the tables, mutates the clone, and atomically publishes it, keeping the
old snapshot alive until any in-flight detour is done with it. Reads are never blocked; only writes serialize,
and writes are rare.

## Fault isolation

A mod callback runs arbitrary code, and a buggy one can dereference bad memory. That's an access violation - a
*structured* exception, which a normal C++ `try/catch` does not catch. Without protection, one bad mod would
crash the whole game. So Kuru calls each callback inside a structured-exception guard: a fault is caught,
logged with the function that triggered it, and the game keeps running.

## The event bus on top

The [event bus](../guides/events.md) is a thin layer over this engine. A `Pre`/`PostFunctionCallEvent` listener
is registered as an ordinary hook on the named function - it reuses everything here, canonical routing and
dedup included. Module-defined events don't touch the game at all; they go through a separate listener registry
that fans out on `Dispatch`.
