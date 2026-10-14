# Regenerating the SDK

The generated SDK bakes Rocket League's offsets into typed headers (see
[the reflected SDK](../modding/concepts/10-the-reflected-sdk.md)). Game updates may change them, regenerate
and verify against the current game before distributing mods. Core reflection layouts and the loader's
own generated-header dependencies also need checking after an update.

## Regenerating

At the main menu, eject an existing Kuru loader first, then use **Developer → Generate SDK** in the
injector. Ejecting follows the isolated workflow tested for this release, not a dependency check
required by SDKGen. Source builders can inject their freshly built `KuruSDKGen.dll` directly. The generator
reports progress in its log and completion/failure with message boxes. It walks reflected types,
builds an in-memory model, generates headers, and packages a standalone mod template.

The generator remains loaded after its worker finishes. To run a newly rebuilt generator reliably,
restart the game rather than assuming reinjecting the same DLL reruns its worker.

## Checking it worked

When it finishes you get a "SDK generated" message box, and next to the DLL you will find:

- `generated-sdk/` - the headers (`KuruSDK.hpp`, `Kuru/`, `Generated/`). Check the completion message too:
  headers can be generated successfully while standalone-template packaging fails.
- `mod-template/` - a standalone mod project with the generated headers, SDK runtime, and matching ImGui.
  Build it with `cmake -B build -A x64` followed by `cmake --build build --config Release`.
  Existing template folders are preserved; later generations use numbered names.
- The current generator keeps its reflection model in memory; it does not write `dump.json`.
- `KuruSDKGen.log` - a detailed log of the walk. This is where to look if something went wrong; a
  `GNames pattern not found` line there means the dumper's byte pattern is stale for this game version (see
  [finding patterns](finding-patterns.md)).

Then confirm the SDK itself looks right:

- The log shows plausible names/object counts and reflected types. Counts vary with version and session;
  a low count can indicate early injection or invalid resolution, rather than a fixed required total.
- Classes you know are present: `Car_TA`, `Ball_TA`, `PlayerController_TA`.
- Spot-check a few offsets against live values. Pick a field, read it through the Objects tab on a live object,
  and confirm the generated struct puts that field at the same offset.

## Rebuilding mods

Mods compile against the SDK, so after regenerating, rebuild any mod you maintain against the new headers and
ship the new build. A mod using stale layouts can read the wrong bytes if those fields changed.
Copy or migrate your mod sources into the fresh template, retaining the matching bundled SDK/ImGui.
Existing template folders are not overwritten automatically.

`KuruSDKGen.log` appends across attempts; old errors can remain above a later successful run.
