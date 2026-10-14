# Kuru::Game

Helpers for common game objects. See the
[reading game state guide](../modding/guides/reading-game-state.md). Pointer getters can return null,
collections return vectors, and the clock has a separate missing-value result.
Use these helpers on the game-thread dispatcher with game headers matching the current build.

## Functions

- `GetEngine() -> UEngine*`: the game engine.
- `IsValid(UObject* object) -> bool`: checks readable index and identity in GObjects. It does not pin the
  object, check every destruction flag, or make later access safe across threads or frames.
- `IsInGame() -> bool`: whether `GetGameEvent()` is non-null. This can also be true outside a competitive match.

- `GetLocalPlayer(int index = 0) -> ULocalPlayer_TA*`: a local player.
- `GetPlayerController(int index = 0) -> APlayerController_TA*`: a local player's controller.
- `GetLocalPawn() -> APawn*`: your pawn.
- `GetLocalCar() -> ACar_TA*`: your car.
- `GetLocalPRI() -> APRI_TA*`: your player replication info (name, score, team).
- `GetHUD() -> AHUD*`: the HUD.
- `GetCamera() -> ACamera*`: the player camera.

- `GetBall() -> ABall_TA*`: first ball in the soccar event, falling back to a global type scan.
- `GetCars() -> std::vector<ACar_TA*>`: every car in the match.
- `GetPRIs() -> std::vector<APRI_TA*>`: every player's replication info.

- `GetGameEvent() -> AGameEvent_TA*`: the current match.
- `GetSoccarGameEvent() -> AGameEvent_Soccar_TA*`: the match as a soccar game, when it is one.
- `GetSecondsRemaining() -> int`: seconds left on the soccar clock, or `-1` without a soccar event.

- `GetAllOfType<T>() -> std::vector<T*>`: all non-template objects of a given type in GObjects.
- `GetFirstOfType<T>() -> T*`: the first non-template object of a given type in GObjects, or null.

The generic type helpers scan GObjects and exclude templates. Results can include objects outside
the current match. Common player/match helpers follow pointers from a cached
engine root instead. Returned object pointers are borrowed engine state.
