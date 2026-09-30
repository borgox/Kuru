# Regenerating the SDK

The generated SDK bakes Rocket League's offsets into typed headers (see
[the reflected SDK](../modding/concepts/10-the-reflected-sdk.md)). Every game update shifts offsets, so after
one you regenerate it. The loader itself does not need regenerating; it resolves the engine at runtime.

## The two steps

Today generation is two steps:

1. **Dump.** Build and inject the dumper (`KuruDump`) into a fully loaded game, at the main menu. It walks
   `GObjects` and writes every class, property, and function, with names, kinds, offsets, and flags, to a dump
   file.
2. **Generate.** Run the offline generator (`KuruSDKGen`) on that dump together with `sdk-base`. It emits the
   typed headers into `generated-sdk`.

An injectable generator that does both in one in-process pass is planned; until it lands, this is the flow.

## Sanity-checking the result

Before trusting a fresh SDK, confirm it looks right:

- The dump's object count is in the expected range, roughly 180,000 or more in a running game. A tiny count
  means the dump ran before the game was ready.
- Classes you know are present: `Car_TA`, `Ball_TA`, `PlayerController_TA`.
- Spot-check a few offsets against live values. Pick a field, read it through the Objects tab on a live object,
  and confirm the generated struct puts that field at the same offset.

## Rebuilding mods

Mods compile against the SDK, so after regenerating, rebuild any mod you maintain against the new headers and
ship the new build. A mod built against the previous version reads the wrong offsets on the updated game.
