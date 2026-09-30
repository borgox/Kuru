# Your first mod

The template's `src/MyMod.cpp` is already a complete mod:

```cpp
#include <Kuru/Kuru.hpp>

class MyMod final : public Kuru::IMod {
public:
    bool OnLoad() override {
        Kuru::Info("MyMod loaded");

        Kuru::Commands::Register("mymod", "mymod - example command", [](const char*) {
            Kuru::Info("hello from MyMod");
        });

        Kuru::Hooks::Post("Function TAGame.Ball_TA.OnHitGoal", [](Kuru::HookContext&) {
            Kuru::Info("goal scored");
        });
        return true;
    }
};

KURU_EXPORT_MOD(MyMod, "MyMod", "your name", "0.1.0")
```

Piece by piece:

- `#include <Kuru/Kuru.hpp>` - the one header that pulls in the whole mod API.
- `class MyMod : public Kuru::IMod` - your mod is an `IMod`. Override the callbacks you need, here it's
  just `OnLoad`.
- `Kuru::Info(...)` - writes a line to the F3 console and the log.
- `Kuru::Commands::Register("mymod", ...)` - adds a console command. Type `mymod` in F3 to run it.
- `Kuru::Hooks::Post("Function TAGame.Ball_TA.OnHitGoal", ...)` - runs your callback right after the
  game's `OnHitGoal`, i.e. every time a goal is scored.
- `KURU_EXPORT_MOD(MyMod, "MyMod", "your name", "0.1.0")` - the macro that makes the DLL a Kuru mod: it
  wires up the name, author, and version Kuru shows in the menu.

That's a working mod. From here the [guides](../guides) cover hooks, reading game state, the GUI, and
events in depth.

## Next

[Build and load it](building-and-loading.md).
