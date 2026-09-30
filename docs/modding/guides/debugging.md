# Debugging

## Logging

`Kuru::Info`, `Kuru::Warning`, and `Kuru::Trace` write to the F3 console and to the log file. Lines are
tagged with your mod's name.

```cpp
Kuru::Info("value is " + std::to_string(x));
```

The log file is `kuru.log` in `%AppData%/Roaming/Kuru`.

## The console

Press F3 for the console. It shows every mod's log lines - filter by level or by mod tag - and runs
commands, both your own and Kuru's built-ins like `set` / `get`.

## The dev tabs

The F2 menu has tools for figuring out the game:

- **Objects** - search live objects and classes, inspect and edit their properties.
- **Functions** - record live function calls with their parameters; use it to find what to hook.

## DebugView

Kuru also writes to Windows debug output, so
[DebugView](https://learn.microsoft.com/sysinternals/downloads/debugview) catches everything - including the
early bootstrap lines before the console exists. Run it as admin with "Capture Global Win32" on. This is
where you look when a mod doesn't load and you need to see why.
