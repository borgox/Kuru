# Kuru mod template

A minimal Kuru mod. Copy this folder, rename it, and edit `src/MyMod.cpp`.

## Prerequisites

- Visual Studio 2022 (or the Build Tools) with the C++ workload
- CMake 3.24+
- A Kuru checkout that has been built once (that build produces the generated SDK the mod compiles against)

## Build

```
cmake -B build -DKURU_DIR=C:/path/to/Kuru
cmake --build build --config Release
```

`KURU_DIR` points at your Kuru source tree. The generated SDK is picked up automatically from its build
directory; if it can't be found, pass `-DKURU_GENERATED_SDK=C:/path/to/generated-sdk`.

The result is `build/Release/MyMod.dll`. Drop it in Kuru's `mods/` folder and inject.
