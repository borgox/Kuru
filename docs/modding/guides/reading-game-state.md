# Reading game state

Kuru generates a typed SDK from Rocket League, so you read the game's objects like normal C++. The
`Kuru::Game::` helpers are the shortcut to the objects you'll reach for most.

## The helpers

```cpp
if (Kuru::Game::IsInGame()) {
    int seconds = Kuru::Game::GetSecondsRemaining();
    Kuru::RL::ACar_TA* car = Kuru::Game::GetLocalCar();
    Kuru::RL::ABall_TA* ball = Kuru::Game::GetBall();
}
```

Common ones:

- `IsInGame()` - is the viewport's game-event pointer non-null.
- `GetLocalCar()`, `GetLocalPawn()` - your car.
- `GetBall()`, `GetCars()` - the ball, and every car.
- `GetPlayerController()`, `GetLocalPRI()` - your controller and player info.
- `GetGameEvent()`, `GetSecondsRemaining()` - the match and its clock.

Pointer helpers can return `nullptr`; collection helpers return vectors and the clock helper returns
`-1` without a soccar event. `GetBall` falls back to a global scan, so it need not be a ball from the
current match. `IsValid` checks index/pointer identity in GObjects; it does not pin an object or make
cross-thread access safe. Reacquire objects on the dispatcher rather than keeping old pointers.

Run these calls from `OnUpdate` or queued work, not directly from initial `OnLoad`, UI handlers, or workers.
See [threading](threading.md) for how Kuru identifies its dispatcher thread.

## Reading fields

The objects are typed SDK classes - you read their fields directly. To see what a class exposes, open the
**Objects tab** in the F2 menu, search for it (e.g. `Car_TA`), and browse its properties, or read the
generated SDK headers.

## Beyond the helpers

The helpers cover the common objects. For anything else, the SDK can find and walk any object in the game
by class or name - see the [typed SDK reference](../../api-reference/sdk.md) and
[the object system](../concepts/02-the-object-system.md).
