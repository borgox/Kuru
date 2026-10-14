# Building from source

Kuru is Windows-only and builds with CMake and MSVC. Most people don't need this - grab a release
instead. Build from source if you want to change the loader itself.

## Prerequisites

- Windows 10 or 11 (x64)
- Visual Studio 2022 with the "Desktop development with C++" workload (or the Build Tools)
- CMake 3.25 for the checked-in presets; manual configuration supports 3.24
- Git for fetched dependencies and Python 3 for embedding/packaging scripts, available on PATH

## Generate the SDK, then build the injector

```
git clone https://github.com/borgox/Kuru.git
cd Kuru
cmake --preset msvc-x64-debug
cmake --build build/msvc-x64-debug --config Release --target KuruSDKGen
```

Inject `build/msvc-x64-debug/libs/KuruSDKGen/Release/KuruSDKGen.dll` into Rocket League to generate
the reflected SDK. The loader compiles typed cursor helpers against these generated headers and
cannot build before that step succeeds. Then build the injector and embedded DLLs:

```
cmake --build build/msvc-x64-debug --config Release --target KuruInjector
```

MinHook, Dear ImGui, and the icon headers are fetched at pinned commits on first configuration.
The icon font is a separate unpinned CDN download; failure disables those font glyphs.

The result is:

```
build/msvc-x64-debug/injector/Release/Kuru.exe
```

`Kuru.exe` embeds both DLLs and normally extracts them under `%APPDATA%\Kuru\bin` on launch.
Use `Release` for injected builds. The preset's directory is named `msvc-x64-debug`, but `--config Release`
selects Release with this multi-configuration Visual Studio generator.

## Packaging a release

After building `KuruInjector` in Release, run from the repository root:

```powershell
cpack --config build/msvc-x64-debug/CPackConfig.cmake -C Release -B build/releases
```

The ZIP contains `Kuru.exe`, `LICENSE`, and `THIRD_PARTY_LICENSES`. The executable also embeds
both license files and displays them in its Licenses tab, so notices remain accessible when
the executable is copied on its own. Update the package version in the root `CMakeLists.txt`
alongside the injector version before publishing a new version.

## Other targets

`KuruSDKGen` is the injectable SDK generator (a separate DLL). It can be built without the generated
headers. The loader expects its output under `build/msvc-x64-debug/libs/KuruSDKGen/Release/generated-sdk`.
For an SDK elsewhere, configure with `-DKURU_GENERATED_SDK="C:/path/to/generated-sdk"`.

`KuruLoader` builds just `libs/KuruLoader/Release/KuruLoader.dll`. `KuruInjector` embeds that DLL and
`KuruSDKGen.dll`, so a fresh source checkout must complete the SDK generation step before building it.
Mod authors using a generated standalone template do not need this source-build workflow.
