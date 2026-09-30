# Kuru::Game

Shortcuts to the game objects you reach for most. See the
[reading game state guide](../modding/guides/reading-game-state.md). Every getter returns null when the thing
does not exist (in a menu, no ball yet), so check before you use it.

## Functions

`GetEngine() -> UEngine*`  the game engine.
`IsValid(UObject* object) -> bool`  whether an object pointer is still a live, valid object.
`IsInGame() -> bool`  whether a match is running.

`GetLocalPlayer(int index = 0) -> ULocalPlayer_TA*`  a local player.
`GetPlayerController(int index = 0) -> APlayerController_TA*`  a local player's controller.
`GetLocalPawn() -> APawn*`  your pawn.
`GetLocalCar() -> ACar_TA*`  your car.
`GetLocalPRI() -> APRI_TA*`  your player replication info (name, score, team).
`GetHUD() -> AHUD*`  the HUD.
`GetCamera() -> ACamera*`  the player camera.

`GetBall() -> ABall_TA*`  the ball.
`GetCars() -> std::vector<ACar_TA*>`  every car in the match.
`GetPRIs() -> std::vector<APRI_TA*>`  every player's replication info.

`GetGameEvent() -> AGameEvent_TA*`  the current match.
`GetSoccarGameEvent() -> AGameEvent_Soccar_TA*`  the match as a soccar game, when it is one.
`GetSecondsRemaining() -> int`  seconds left on the clock.

`GetAllOfType<T>() -> std::vector<T*>`  every live object of a given type.
`GetFirstOfType<T>() -> T*`  the first live object of a given type, or null.
