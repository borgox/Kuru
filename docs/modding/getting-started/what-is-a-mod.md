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

Mods are native C++20 code inside the game process. A mod can corrupt or crash the game; host
guards provide only the limited [failure handling](../../api-reference/mod.md#failure-handling).

Kuru attempts `.dll` files in its `mods` folder, skips `.dll.disabled` files, and logs rejected DLLs.
Failed initial loads do not appear in the Mods tab and need eject/reinject or restart for another attempt. Your mod is a class that implements `Kuru::IMod`, and Kuru
calls into it at the right times:

- `OnLoad` - on initial load and when enabled again. Register hooks, commands, and settings here.
- `OnUpdate(dt)` - when the game-thread dispatcher pumps an update.
- `OnRender` - while the overlay builds an ImGui frame, on the game-window thread.
- `OnUnload` - before disabling, reloading an enabled instance, or ejecting an enabled mod.

Initial loading happens on Kuru's bootstrap worker; it is not a game-thread callback. Queue game
object work with `Kuru::Execute`. See [threading](../guides/threading.md) and the
[lifecycle reference](../../api-reference/mod.md) before sharing state between callbacks.

That's the whole shape. Everything else is the API you call from inside those.

## Next

Before you can build one you need a compiler and the Kuru SDK - [environment setup](environment-setup.md).
