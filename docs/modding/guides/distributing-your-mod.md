# Distributing your mod

A mod is a single `.dll`. To share it, hand people the DLL - they drop it in Kuru's `mods/` folder next to
the loader and re-inject.

## Build for release

Build `Release`, not `Debug` - it matches the game's runtime and it's what users run:

```
cmake --build build --config Release
```

Ship `build/Release/YourMod.dll`.

## Version compatibility

A mod is compiled against a version of the Kuru SDK. Kuru refuses to load a mod built for a newer version
than the running loader (and says so in the log), so build against the version you target and note it with
your download. Older mods keep working under a newer loader.

## Be a good citizen

- Prefix your cvar and command names (`yourmod_`) so they don't clash with other mods.
- If you hook a base-class function and Kuru warns about an override, hook the name it suggests rather than
  suppressing the warning blindly.
- Keep `OnRender` and hook callbacks cheap - they run every frame or every call.
