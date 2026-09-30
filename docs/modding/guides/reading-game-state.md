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

- `IsInGame()` - is a match running.
- `GetLocalCar()`, `GetLocalPawn()` - your car.
- `GetBall()`, `GetCars()` - the ball, and every car.
- `GetPlayerController()`, `GetLocalPRI()` - your controller and player info.
- `GetGameEvent()`, `GetSecondsRemaining()` - the match and its clock.

They return `nullptr` when the thing doesn't exist (you're in a menu, there's no ball yet), so check before
you use them. `Kuru::Game::IsValid(obj)` guards against stale objects too.

## Reading fields

The objects are typed SDK classes - you read their fields directly. To see what a class exposes, open the
**Objects tab** in the F2 menu, search for it (e.g. `Car_TA`), and browse its properties, or read the
generated SDK headers.

## Beyond the helpers

The helpers cover the common objects. For anything else, the SDK can find and walk any object in the game
by class or name - see the [API reference](../../api-reference) and
[the object system](../concepts/02-the-object-system.md).
