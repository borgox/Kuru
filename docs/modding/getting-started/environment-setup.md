# Environment setup

Writing a mod needs a C++ toolchain and the self-contained template produced by Kuru's SDK generator.

## Prerequisites

- Windows 10 or 11 (x64)
- Visual Studio 2022 with the "Desktop development with C++" workload (or the Build Tools)
- CMake 3.24 or newer
- C++20 familiarity with classes, lambdas, pointers, and ownership

## Get the SDK

At Rocket League's main menu, use **Developer → Generate SDK** in the injector. If the loader is
already injected, eject it first to follow the isolated workflow tested for this release. This is
not a required dependency check in SDKGen. Alongside `generated-sdk`, the generator writes a `mod-template`
folder containing the generated game headers, SDK runtime sources, and matching ImGui sources.
The completion dialog shows the folder's location. If it already exists, a numbered folder is created
instead, preserving any mod code you have edited.

The template files, SDK runtime, and matching ImGui sources come from the generator DLL itself;
the freshly reflected game headers are copied into that folder. There are no paths back to the
developer's Kuru checkout.

## Start from the template

Copy the entire generated `mod-template` folder to your development directory. Open a Developer
PowerShell for Visual Studio there and build:

```
cmake -B build -A x64
cmake --build build --config Release
```

`build/Release/MyMod.dll` is your mod. Drop it in Kuru's `mods/` folder next to the loader.
No Kuru checkout, SDK path settings, Python, or dependency downloads are needed to build it.

## Next

[Your first mod](your-first-mod.md) walks through what's in the template.

The default template location is `%APPDATA%\Kuru\bin\mod-template`, with numbered alternatives
when that folder already exists. A custom generator DLL writes beside its own location instead.
