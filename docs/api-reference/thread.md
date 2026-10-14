# Kuru task scheduling

Schedule work on Kuru's dispatcher. See the [threading guide](../modding/guides/threading.md). These are in the
`Kuru` namespace directly (`Kuru::Execute`, not `Kuru::Thread::Execute`).

## Functions

### `Execute(std::function<void()> fn)`

Queue `fn` for a later dispatcher pump. Calls from the dispatcher are queued too.
Use it from initial `OnLoad`, `OnRender`, console commands, or worker threads to schedule game-object work.

### `SetTimeout(std::function<void()> fn, float seconds)`

Run `fn` once when a dispatcher pump observes that the delay has elapsed. A nonpositive delay uses the
immediate queue. Execution waits for the dispatcher, so the delay can exceed the requested interval.

### `IsGameThread() -> bool`

Whether the current thread matches the most recent dispatcher pump. Before the first pump it returns
false. This is Kuru's recorded dispatch thread, not an independent check of Rocket League's main thread.
See [threading](../modding/guides/threading.md).

## Lifetime

Pending tasks are canceled when their mod is disabled, reloaded, rejected during loading, or
ejected. Cancellation also applies to tasks waiting in a dispatch batch. A callback
that has started running is allowed to finish. Callback storage is released after execution,
cancellation, or rejection during shutdown.

The current SDK includes the cleanup-aware queue introduced in host API v3. Older mods retain the legacy
API: work scheduled from host-invoked mod callbacks is associated with that mod, but the old
API has no cleanup callback for canceled storage. Legacy work submitted by a mod's own worker
thread has no owner context; rebuild those mods before relying on automatic cancellation.

The current export macro stores task ownership per mod instance, so `Execute`/`SetTimeout` from its
workers are owned too. Stop and join those workers in `OnUnload`; task cancellation does not stop
the threads themselves. Do not keep callback parameter/result pointers for later queued work.

Ordinary C++ exceptions in tasks are caught without a log line. Access violations are not guarded
here; see [failure handling](mod.md#failure-handling).
