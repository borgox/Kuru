# Kuru

A mod loader and C++ modding SDK for (offline) Rocket League.

Kuru injects into Rocket League, hooks its UnrealScript and native functions, and gives mods a C++ API
to read and edit game state, react to events, draw overlays, and register console commands. It resolves
what it needs from the game at runtime, so it keeps working across game updates.

![Kuru's in-game overlay: console and object browser](docs/images/menu.webp)

## Features

- Hook any UnrealScript or native function, and read, edit, or cancel the call.
- A reflected SDK generated from the game - typed access to Rocket League's classes.
- In-game overlay (F2) and console (F3), with built-in object and function browsers.
- Console commands, persistent settings, and a typed event system for mods.
- Runtime resolution of the game's internals - no hardcoded addresses to break on updates.

## Install

1. Download the latest release. <!-- release link -->
2. Run the Kuru injector and launch Rocket League. <!-- injector flow -->
3. Press F2 for the menu, F3 for the console.

Mods are `.dll` files - drop them in the `mods/` folder next to the loader.

> Kuru is for offline and casual play. Don't use it where it could affect other players or your account.

## Writing a mod

A mod is a small C++ DLL:

```cpp
#include <Kuru/Kuru.hpp>

class MyMod final : public Kuru::IMod {
public:
    bool OnLoad() override {
        Kuru::Hooks::Post("Function TAGame.Ball_TA.OnHitGoal", [](Kuru::HookContext&) {
            Kuru::Info("goal scored");
        });
        return true;
    }
};

KURU_EXPORT_MOD(MyMod, "MyMod", "your name", "0.1.0")
```

Start from `templates/mod-template`, or read the [getting-started guide](docs/modding/getting-started).

## Documentation

- [Getting started](docs/modding/getting-started) - write your first mod
- [Guides](docs/modding/guides) - hooks, the SDK, GUI, events, and more
- [Concepts](docs/modding/concepts) - how Rocket League and Kuru work under the hood
- [API reference](docs/api-reference)
- [Building from source](docs/building.md)

## Built with

- [MinHook](https://github.com/TsudaKageyu/minhook) - function hooking
- [Dear ImGui](https://github.com/ocornut/imgui) - the overlay

Inspired by [BakkesMod](https://bakkesmod.com/) and Toga.

## License

MIT - see [LICENSE](LICENSE).
