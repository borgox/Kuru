# What is a mod?

A Kuru mod is a small C++ DLL that Kuru loads into Rocket League. Once loaded it runs inside the game and
can:

- **Hook functions** - react to what the game does, and read, change, or cancel it. "When a goal is
  scored...", "before the camera updates...".
- **Read and edit game state** - the ball's speed, your car's boost, the score - through Kuru's SDK.
- **Draw an overlay** - its own window in the F2 menu, built with Dear ImGui.
- **Add console commands and settings** - commands you type in the F3 console, and values that persist
  across restarts.
- **Listen for events** - goals, match start, or events other mods (or your own) define.

## How it runs

Kuru loads every `.dll` in its `mods/` folder. Your mod is a class that implements `Kuru::IMod`, and Kuru
calls into it at the right times:

- `OnLoad` - once, when the mod loads. Register your hooks, commands, and settings here.
- `OnUpdate(dt)` - every frame, on the game thread.
- `OnRender` - every frame, to draw your overlay.
- `OnUnload` - when the mod is disabled.

That's the whole shape. Everything else is the API you call from inside those.

## Next

Before you can build one you need a compiler and the Kuru SDK - [environment setup](environment-setup.md).
