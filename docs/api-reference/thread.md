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
