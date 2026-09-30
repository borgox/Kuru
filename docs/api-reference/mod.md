# Kuru::IMod and KURU_EXPORT_MOD

Every mod is a class implementing `Kuru::IMod`, exported with `KURU_EXPORT_MOD`. See
[what is a mod](../modding/getting-started/what-is-a-mod.md).

## IMod

Override the callbacks you need. All of them are optional.

`bool OnLoad()`
Called once when the mod loads. Register hooks, commands, and CVars here. Return false to refuse to load.

`void OnUpdate(float deltaSeconds)`
Called every frame, on the game thread.

`void OnRender()`
Called every frame, to draw your overlay.

`void OnUnload()`
Called when the mod is disabled.

## KURU_EXPORT_MOD

`KURU_EXPORT_MOD(ModType, "Name", "Author", "Version")`
Exports the mod so Kuru can load it. Put it once at file scope. `ModType` is your `IMod` class, and the strings
show up in the Mods tab.
