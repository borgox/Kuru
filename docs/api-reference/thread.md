# Kuru::Thread

Run work on the game thread. See the [threading guide](../modding/guides/threading.md). These are in the
`Kuru` namespace directly (`Kuru::Execute`, not `Kuru::Thread::Execute`).

## Functions

`Execute(std::function<void()> fn)`
Queue `fn` to run on the next game-thread tick. Use it when you are on another thread (like `OnRender`) and
need to touch game objects safely.

`SetTimeout(std::function<void()> fn, float seconds)`
Run `fn` once after a delay, on the game thread.

`IsGameThread() -> bool`
Whether the current thread is the game thread.

## Lifetime

Pending tasks are canceled when their mod is disabled, reloaded, rejected during loading, or
ejected. Cancellation also applies to tasks already waiting in a dispatch batch; a callback
that has started running is allowed to finish. Callback storage is released after execution,
cancellation, or rejection during shutdown.

Rebuild mods against host API v3 to use the cleanup-aware queue. Older mods retain the legacy
API: work scheduled from host-invoked mod callbacks is associated with that mod, but the old
API has no cleanup callback for canceled storage. Legacy work submitted by a mod's own worker
thread has no owner context; rebuild those mods before relying on automatic cancellation.
