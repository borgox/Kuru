# Building from source

Kuru is Windows-only and builds with CMake and MSVC. Most people don't need this - grab a release
instead. Build from source if you want to change the loader itself.

## Prerequisites

- Windows 10 or 11 (x64)
- Visual Studio 2022 with the "Desktop development with C++" workload (or the Build Tools)
- CMake 3.24 or newer

## Build the loader

```
git clone https://github.com/borgox/Kuru.git
cd Kuru
cmake --preset msvc-x64-debug
cmake --build build/msvc-x64-debug --config Release --target KuruLoader
```

Dependencies (MinHook, Dear ImGui) are fetched automatically the first time you configure.

The result is:

```
build/msvc-x64-debug/libs/KuruLoader/Release/KuruLoader.dll
```

That is the DLL you inject. Build `Release` for anything you inject - it matches the game's runtime.

## Other targets

`KuruSDKGen`, `KuruSDKGenRuntime`, and `KuruDump` make up the SDK-generation toolchain. Building the
loader doesn't need them.
