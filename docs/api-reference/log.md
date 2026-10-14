# Kuru logging

Logging functions live directly in `Kuru`. With the host API initialized they write tagged lines to
F3 and `%APPDATA%\Kuru\kuru.log`. See [debugging](../modding/guides/debugging.md).

| Function | Meaning |
| --- | --- |
| `Trace(std::string_view)` | Detailed diagnostic line |
| `Info(std::string_view)` | Normal message |
| `Warning(std::string_view)` | Warning |
| `Error(std::string_view)` | Error |
| `Log(LogLevel, std::string_view)` | Explicit level; the four helpers above delegate to it |

`LogLevel` contains Trace, Info, Warning, and Error. The console initially shows Trace and above.
Its level/tag filters affect display only; all levels are written to the file. The in-memory console
keeps up to 4000 entries. When the file sink first opens a log over 5 MiB, it deletes the old file
rather than archiving it. `clear` only clears the in-memory history.

`SetLogger(LogFunc)`, where `LogFunc` is `void(*)(LogLevel, const char*)`, replaces this mod's SDK
logging callback. Host initialization installs a callback that forwards to the shared tagged log.
Replacing it afterward diverts this mod's output from that route. With no callback, the SDK uses
Windows debug output. Keep a custom callback alive while installed; ordinary mods need no override.
