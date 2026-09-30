# Regenerating the SDK

The generated SDK bakes Rocket League's offsets into typed headers (see
[the reflected SDK](../modding/concepts/10-the-reflected-sdk.md)). Every game update shifts offsets, so after
one you regenerate it. The loader itself does not need regenerating; it resolves the engine at runtime.

## Regenerating

Inject `KuruSDKGenRuntime` into a fully loaded game, at the main menu (Xtreme Injector, or any injector). It
has no UI of its own; it runs once and reports with message boxes. In one pass it walks `GObjects`, dumps
every class, property, and function (names, kinds, offsets, flags), and generates the typed headers.

## Checking it worked

When it finishes you get a "SDK generated" message box, and next to the DLL you will find:

- `generated-sdk/` - the headers (`KuruSDK.hpp`, `Kuru/`, `Generated/`). This appearing is the real success
  signal.
- `dump.json` - the raw reflection dump, tens of MB with the full object set.
- `KuruDump.log` - a detailed log of the walk. This is where to look if something went wrong; a
  `GNames pattern not found` line there means the dumper's byte pattern is stale for this game version (see
  [finding patterns](finding-patterns.md)).

Then confirm the SDK itself looks right:

- The dump's object count is in the expected range, roughly 180,000 or more in a running game. A tiny count
  means the dump ran before the game was ready.
- Classes you know are present: `Car_TA`, `Ball_TA`, `PlayerController_TA`.
- Spot-check a few offsets against live values. Pick a field, read it through the Objects tab on a live object,
  and confirm the generated struct puts that field at the same offset.

## Rebuilding mods

Mods compile against the SDK, so after regenerating, rebuild any mod you maintain against the new headers and
ship the new build. A mod built against the previous version reads the wrong offsets on the updated game.
