# Troubleshooting

Start with `%APPDATA%\Kuru\kuru.log`. The injector's success message means LoadLibrary completed,
not that every bootstrap stage or mod succeeded. SDK generation uses `KuruSDKGen.log` beside the
configured generator DLL. Its log appends, so distinguish old attempts from the current one.

| Symptom or log fragment | Next check |
| --- | --- |
| F2 does nothing | Check the configured loader path, whether the game is running, and the current bootstrap log. Try F3. Do not inject another loader over a partially initialized one. |
| `HostThread: game globals not ready` | Wait at the main menu. Restart the game before another attempt if a loader remains mapped. |
| `global bootstrap failed: GNames/GObjects not resolved` | Validate the current game's globals data signature and layouts; see [patterns](maintaining/finding-patterns.md). |
| `game allocator unavailable` | Required allocator binding failed; bootstrap stops. Check the current game build and allocator resolver. |
| `no mods directory at` | Install a mod beside the configured loader, in its `mods` subfolder, before injection. |
| `not a Kuru mod (missing exports)` | Check that the DLL uses `KURU_EXPORT_MOD` and is a compatible x64 mod. |
| `skipping mod built for a newer Kuru` | Use a loader supporting the mod's required host API. The version check does not validate game layouts. |
| `RegisterHook: function not found` | Verify the exact full name and that its class is loaded. See [hook diagnostics](modding/guides/hooks.md#my-hook-never-fires). |
| `interpreter globals: runtime resolve FAILED` | The loader used build-specific fallback RVAs. Verify them before relying on native argument editing. |
| `hook callback faulted (suppressed)` | Identify the mod and function, disable the hook, and check pointer lifetimes and parameter layout. |
| Copy/link of a mod DLL fails | Eject first: a loaded DLL is mapped and normally locked against replacement. Reload only recreates the instance. |
| `LoadLibrary` or access error | Check the file, architecture, dependent DLLs, and access rights. No blanket elevation requirement has been established by the project. |
| Raw mouse capture unavailable | The overlay may receive input without swallowing the game's copy. Availability after reinjection is unresolved; restart the game if input leaks. |

If eject reports failure, restart Rocket League before reinjecting. Preserve the log and game/build
information for diagnosis. Do not disable security protections to work around a launcher or injection
restriction; the project has not established an anti-cheat-compatible deployment procedure.
