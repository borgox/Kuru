# Threading

Kuru runs your callbacks on two threads:

- **Game thread** - `OnLoad`, `OnUpdate`, and hook callbacks. Game objects are safe to read and write here.
- **Render thread** - `OnRender`. Draw here, but don't touch game objects.

## Hopping to the game thread

If you're on the render thread (or any other thread) and need to touch the game, queue the work with
`Kuru::Execute` - it runs on the next game-thread tick:

```cpp
Kuru::Execute([] {
    if (auto* car = Kuru::Game::GetLocalCar()) {
        // safe here
    }
});
```

`Kuru::IsGameThread()` tells you where you are. `Kuru::SetTimeout(fn, seconds)` runs something later, on the game thread.

## The common pattern

Read game state in `OnUpdate`, store it in your mod, and display the stored value in `OnRender`. That keeps
every game-object access on the game thread.
