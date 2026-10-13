# Kuru::Commands

Register console commands, runnable from the F3 console. See the
[commands guide](../modding/guides/commands-and-cvars.md).

A command handler is `void(const char* args)`, where `args` is the rest of the typed line.

## Functions

`Register(const char* name, const char* help, handler)`
Add a command. `name` is what you type in the console, `help` is the one line shown in the command list, and
`handler` runs when the command is entered.

## Built-in eject command

Type `eject` in the F3 console to shut down Kuru and unload its DLL. Rocket League keeps running.
The overlay stops, pending Execute/timeouts are canceled, enabled mods receive OnUnload on the
existing game-thread dispatcher, and mod DLLs are released. The window procedure and detours
are restored before the loader is unloaded. Disabled mods are destroyed without a second OnUnload.

Mods must stop and join their own background workers in OnUnload and must not call the host
after teardown returns. Kuru cannot terminate arbitrary mod-owned threads safely.

If the window cannot be detached, callbacks do not drain, or teardown fails/times out, Kuru
stays resident with shutdown requested. Check `%APPDATA%\Kuru\kuru.log` for the failure and
restart Rocket League before reinjecting. Turn off the injector's auto-inject option before
testing eject, otherwise it may inject Kuru again.

Shutdown is performed outside DllMain by a worker; unloading the DLL directly from another
injector is not the supported eject path. The command uses the current dispatcher's thread
identity, which still requires verification against the game's actual main thread.
