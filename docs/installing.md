# Installing and updating Kuru

## Before starting

Use offline/local freeplay for testing, not matchmaking. The main menu alone does not mean the
game is disconnected. No anti-cheat-compatible deployment procedure is established here.

## First launch

1. Put the release's `Kuru.exe` in a folder you can write to and run it.
2. Check the loader and generator paths. By default their embedded DLLs are extracted to
   `%APPDATA%\Kuru\bin`; no source checkout is required.
3. Launch Rocket League and wait at the main menu. Inject the loader.
4. Press F2 for the overlay and F3 for the console. Use offline freeplay to check your mods.

The injector can use custom DLL paths; those paths override the defaults. Its mod installer targets
the `mods` folder beside the configured loader DLL. Drag a mod DLL onto the injector or use its
install option before injecting. Files ending in `.dll.disabled` are not loaded.

## What is stored where

| Location | Contents |
| --- | --- |
| `%APPDATA%\Kuru\injector.ini` | Injector paths, auto-inject preference, and window settings |
| `%APPDATA%\Kuru\config.cfg` | Saved CVars, including values belonging to inactive mods |
| `%APPDATA%\Kuru\kuru.log` | Loader and mod logs |
| `%APPDATA%\Kuru\imgui.ini` | Overlay window positions and sizes |
| `%APPDATA%\Kuru\data\<mod>` | Per-mod data folders |
| Beside the configured loader | `mods` directory |
| Beside the configured generator | Generated SDK, standalone template, and generation log |

`%APPDATA%` already names the user's Roaming AppData directory.

## Replacing a mod DLL

Turn auto-inject off, run `eject` in F3, and wait for the overlay to disappear. Replace the mod DLL
through the installer or file system, then inject again. If eject reports a failure, restart Rocket
League before replacing files or reinjecting. F2's **Reload** recreates the mod instance from the
DLL already loaded in memory; it does not read a rebuilt DLL from disk.

## Updating Kuru

Eject or close Rocket League and close the old injector before using a new `Kuru.exe`. Verify the
configured DLL paths, especially if they point to custom builds. The injector currently skips
extracting an embedded DLL if an existing file has the same size. If an update appears stale, move
the old default loader/generator DLLs aside while they are unloaded and relaunch the injector.
Preserve the adjacent `mods` folder and your settings.

Current SDK wrappers require host API v4 for cleanup-aware hook, event, and command registration.
Rebuild mods against the current SDK to use that behavior. The host API version check does not
validate a mod's generated game layouts or ImGui compatibility.

## Unloading

Use `eject` in F3. Successful eject removes Kuru and its mods while Rocket League continues running.
Mods must stop their own workers in `OnUnload`. See the [eject reference](api-reference/commands.md).

If raw mouse capture is unavailable, the overlay can receive input while the game also receives it.
Restart the game if input leaks after reinjection. See [troubleshooting](troubleshooting.md).
