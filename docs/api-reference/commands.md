# Kuru::Commands

Register commands for the F3 console. See the
[commands guide](../modding/guides/commands-and-cvars.md).

A command handler is `void(const char* args)`, where `args` is the rest of the typed line.

## Functions

### `Register(const char* name, const char* help, handler)`

Add a command. `name` is what you type in the console, `help` is the one line shown in the command list, and
`handler` runs when the command is entered.

Registering an existing name replaces that command. API v4 releases the replaced handler's captured
storage after any active invocation finishes. Commands belong to the current host-invoked mod context
and are removed when the mod is disabled, reloaded, or ejected. Register from `OnLoad` or another
callback invoked by the host for your mod. Direct registration from a worker has no mod ownership
context. Console handlers run on the window thread. Queue game work with `Kuru::Execute`.

## Built-in console commands

Enter these in the F3 console. `help` also lists commands registered by currently loaded mods.

| Command | What it does |
| --- | --- |
| `help` | Lists registered commands and their help text. |
| `clear` | Clears the on-screen console history. The log file is preserved. |
| `set <cvar> <value>` | Sets a CVar and saves it to `config.cfg`. Creates the variable if needed. The value is the rest of the line after the name. |
| `get <cvar>` | Prints an active CVar's value, or an empty value if none is registered. |
| `unset <cvar>` | Deletes the active registration and saved value. |
| `cvarlist` | Lists active CVars, their values, and descriptions where provided. Saved values for inactive mods are not listed. |
| `eject` | Shuts down Kuru and unloads its mods and loader DLL. See below. |

Mod commands appear alongside these built-ins and can replace them by name. Rocket League engine
console commands are separate.

## Eject

Type `eject` in the F3 console to shut down Kuru and unload its DLL. Rocket League keeps running.
Shutdown stops the overlay, cancels pending tasks, and calls `OnUnload` for enabled mods on the
[dispatcher](../modding/guides/threading.md). It then releases mod DLLs and restores the window
procedure and detours before unloading the loader. Disabled mods do not receive a second `OnUnload`.

Mods must stop and join their own background workers in `OnUnload` and must not call the host
after teardown returns. Kuru cannot terminate arbitrary mod-owned threads safely.

If the window cannot be detached, callbacks do not drain, or teardown fails/times out, Kuru
stays resident with shutdown requested. Check `%APPDATA%\Kuru\kuru.log` for the failure and
restart Rocket League before reinjecting. Turn off the injector's auto-inject option before
testing eject so it does not inject Kuru again.

Use `eject` instead of forcing the DLL to unload through another injector. The shutdown worker runs
outside `DllMain`. If `OnUnload` throws an ordinary C++ exception, Kuru logs it and continues host
cleanup. Access violations and throwing destructors can still crash the process. See
[failure handling](mod.md#failure-handling).

## Names and registration failures

Names are case-sensitive and cannot contain spaces; the console splits at the first space. Avoid
built-in names such as `set` or `eject`, since registering one replaces that built-in too. Registration
returns void and silently does nothing if API v4 support is missing or the loader is stopping.
