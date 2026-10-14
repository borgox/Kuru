# Building and loading

## Build

From your mod folder:

```
cmake -B build -A x64
cmake --build build --config Release
```

The output is `build/Release/MyMod.dll`.

## Load

1. Copy `MyMod.dll` into Kuru's `mods/` folder, next to `KuruLoader.dll`.
2. Launch Rocket League, wait at the main menu, then inject Kuru.
3. Open the F2 menu - your mod appears under the **Mods** tab.

## Check it works

Open the F3 console. You should see the load line:

```
[MyMod] MyMod loaded
```

Type `mymod` and it prints `hello from MyMod`. Score a goal and it prints `goal scored`. If the load line
is missing, check that the DLL is in `mods/` and that you built `Release`.

## The loop

To load changed code, eject Kuru (with auto-inject off), rebuild and replace the DLL, then inject again.
Restart Rocket League if eject reports failure. Windows normally locks mapped DLLs against replacement, so copying/linking over one fails.
See the [replacement procedure](../../installing.md#replacing-a-mod-dll).

F2's **Reload** destroys the current instance and creates a fresh one from the same loaded DLL.
Use it to test initialization and teardown, not to load a newly compiled binary. Disable/re-enable
keeps the same instance and calls `OnUnload`/`OnLoad` again.

The [guides](../../README.md) go deeper from here.
