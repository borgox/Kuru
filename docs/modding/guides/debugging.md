# Debugging

## Logging

`Kuru::Info`, `Kuru::Warning`, `Kuru::Error`, and `Kuru::Trace` write to the F3 console and to the log file. Lines are
tagged with your mod's name.

```cpp
Kuru::Info("value is " + std::to_string(x));
```

The log file is `%APPDATA%\Kuru\kuru.log` (`%APPDATA%` already includes Roaming).

## The console

Press F3 for the console. It shows every mod's log lines - filter by level or by mod tag - and runs
commands, both your own and Kuru's built-ins like `set` / `get`.

## The dev tabs

The F2 menu has tools for figuring out the game:

- **Objects** - search live objects and classes, inspect and edit their properties.
- **Functions** - record live function calls with their parameters; use it to find what to hook.

## DebugView

The bootstrap and hook engine also write selected diagnostic lines to Windows debug output.
DebugView or an attached debugger can help inspect them. This is not a complete mirror of every
mod log line; start with `kuru.log` for loading errors and `KuruSDKGen.log` beside the generator for
SDK generation errors.
