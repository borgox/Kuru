# Building and loading

## Build

From your mod folder:

```
cmake -B build -DKURU_DIR=C:/path/to/Kuru
cmake --build build --config Release
```

The output is `build/Release/MyMod.dll`.

## Load

1. Copy `MyMod.dll` into Kuru's `mods/` folder, next to `KuruLoader.dll`.
2. Inject Kuru and launch Rocket League.
3. Open the F2 menu - your mod appears under the **Mods** tab.

## Check it works

Open the F3 console. You should see the load line:

```
[MyMod] MyMod loaded
```

Type `mymod` and it prints `hello from MyMod`. Score a goal and it prints `goal scored`. If the load line
is missing, check that the DLL is in `mods/` and that you built `Release`.

## The loop

Edit, build, copy into `mods/`, then restart Rocket League and re-inject to load the new build - Kuru
doesn't hot-reload a mod that's already loaded. <!-- TODO: Hot reloading might be a good idea -->

The [guides](../guides) go deeper from here.
