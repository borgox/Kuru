# Threading

Callbacks do not all run on one thread:

| Callback | Current execution path |
| --- | --- |
| Initial `OnLoad` | Kuru's bootstrap worker, before engine detours are enabled |
| `OnUpdate`, `Execute`, timeout | Kuru's game-thread dispatcher |
| Re-enable/reload and their lifecycle calls | Queued to that dispatcher by the Mods tab |
| `OnUnload` during eject | Shutdown work on the recorded dispatcher thread |
| `OnRender`, console handlers | Window thread while building the ImGui frame |
| Hook | The thread making the intercepted engine call |
| Module-event listener | Synchronously on the thread calling `Events::Dispatch` |

The **dispatcher** runs queued tasks and `OnUpdate`. `Present` marks an update pending; the next
`ProcessEvent` detour records its calling thread and runs that work. The API calls this the game
thread, but Kuru does not verify it against Rocket League's main thread. Another thread reaching
`ProcessEvent` first can become the recorded dispatcher thread. `IsGameThread` checks only that
recorded ID.

The **window thread** handles window messages, console handlers, and `OnRender`. The **bootstrap
worker** initializes Kuru and loads mods before engine detours are enabled.

## Scheduling game work

Queue game-object work from initial `OnLoad`, `OnRender`, commands, or workers with `Kuru::Execute`.
It runs on a later dispatcher pump, not immediately:

```cpp
Kuru::Execute([] {
    if (auto* car = Kuru::Game::GetLocalCar()) {
        // Reacquire the current car here; do not capture an old car pointer from the UI.
    }
});
```

`Kuru::SetTimeout(fn, seconds)` waits at least the requested delay before a dispatcher pump can run it.
Hooks and events do not switch threads for you. Keep ImGui calls inside `OnRender`.

## Sharing state with the GUI

Read game state in `OnUpdate`, copy the small values you want to display, and protect that snapshot with
a mutex or suitable atomics before using it in `OnRender`. A plain shared struct/string introduces a
data race if the callbacks run on different threads. Release your own mutex before calling into the
host or game if that call can reenter your mod.

Current SDK tasks carry their owning mod instance and are canceled on disable/reload/failed loading/eject.
That does not stop your own threads: signal and join them in `OnUnload`, and make partial-load failure
paths release resources themselves. See the [task lifetime reference](../../api-reference/thread.md).
