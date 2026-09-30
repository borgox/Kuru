# Environment setup

Writing a mod needs a C++ toolchain and the Kuru SDK. The compiler is the only real install - the SDK is
just headers you point CMake at.

## Prerequisites

- Windows 10 or 11 (x64)
- Visual Studio 2022 with the "Desktop development with C++" workload (or the Build Tools)
- CMake 3.24 or newer

## Get the SDK

The SDK is the `sdk-base` headers plus `generated-sdk` - the typed Rocket League classes, generated from
the game itself. For now, get it by building Kuru once from source (see
[building from source](../../building.md)). That build produces `generated-sdk`, and the template picks it
up automatically. 
> A prebuilt SDK bundle will ship with releases so you won't have to build Kuru yourself. Until then,
> building once from source is the way.

## Start from the template

Copy `templates/mod-template` out of the Kuru repo and rename it. Point it at your Kuru checkout and build:

```
cmake -B build -DKURU_DIR=C:/path/to/Kuru
cmake --build build --config Release
```

`build/Release/MyMod.dll` is your mod. Drop it in Kuru's `mods/` folder next to the loader.

## Next

[Your first mod](your-first-mod.md) walks through what's in the template.
