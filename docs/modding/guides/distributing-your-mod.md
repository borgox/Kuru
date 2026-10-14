# Distributing your mod

A mod is a single `.dll`. To share it, hand people the DLL - they drop it in Kuru's `mods/` folder next to
the loader before injection, or use the injector's mod installer. Eject before replacing a loaded DLL.

## Build for release

Build x64 `Release` for distribution. It uses the release C++ runtime and avoids requiring a debug
runtime on users' machines. It does not by itself establish game-layout or ABI compatibility:

```
cmake --build build --config Release
```

Ship `build/Release/YourMod.dll`.

## Version compatibility

A mod is compiled against a version of the Kuru SDK. Kuru refuses to load a mod built for a newer version
than the running loader (and says so in the log), so build against the version you target and note it with
your download. This checks the host API number, not the Rocket League build or generated layouts.

Older binaries are not guaranteed to work after game or ImGui changes. API v3 introduced owned task
cleanup; API v4 added owned hook/event/command cleanup. Rebuild with the current SDK to benefit from both.
Use the template's bundled ImGui version so it matches the host's shared context.

## Be a good citizen

- Prefix your cvar and command names (`yourmod_`) so they don't clash with other mods.
- If you hook a base-class function and Kuru warns about an override, hook the name it suggests rather than
  suppressing the warning blindly.
- Keep `OnRender` and hook callbacks cheap - they run every frame or every call.
