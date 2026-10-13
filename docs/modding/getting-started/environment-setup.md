# Environment setup

Writing a mod needs a C++ toolchain and the self-contained template produced by Kuru's SDK generator.

## Prerequisites

- Windows 10 or 11 (x64)
- Visual Studio 2022 with the "Desktop development with C++" workload (or the Build Tools)
- CMake 3.24 or newer

## Get the SDK

Run Kuru's SDK generator against the loaded game. Alongside `generated-sdk`, it writes a `mod-template`
folder containing the generated game headers, SDK runtime sources, and matching ImGui sources.
The completion dialog shows the folder's location. If it already exists, a numbered folder is created
instead, preserving any mod code you have edited.

## Start from the template

Copy or rename the generated `mod-template` folder. Open a terminal there and build:

```
cmake -B build -A x64
cmake --build build --config Release
```

`build/Release/MyMod.dll` is your mod. Drop it in Kuru's `mods/` folder next to the loader.
No Kuru checkout, SDK path settings, Python, or dependency downloads are needed to build it.

## Next

[Your first mod](your-first-mod.md) walks through what's in the template.
