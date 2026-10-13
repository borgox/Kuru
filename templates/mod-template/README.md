# Kuru mod template

The SDK generator produces this self-contained folder beside `generated-sdk`.
Copy or rename the entire folder, then edit `src/MyMod.cpp`.
The copy in the Kuru repository is the packaging source; use the generated folder to build a mod.

## Prerequisites

- Windows x64
- Visual Studio 2022 (or the Build Tools) with the C++ workload
- CMake 3.24+

## Build

Run these commands from this folder:

```
cmake -B build -A x64
cmake --build build --config Release
```

No Kuru checkout, SDK paths, dependency downloads, or Python installation are needed.
The folder includes the SDK runtime sources, generated game headers, and Kuru's matching ImGui sources.

The result is `build/Release/MyMod.dll`. Drop it in Kuru's `mods/` folder and inject.
After a game update, generate a fresh template and update your mod's bundled SDK as needed.
Keep ImGui matched to the Kuru host version when updating dependencies.
