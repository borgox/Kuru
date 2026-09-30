# Kuru logging

Write to the F3 console and the log file. These are in the `Kuru` namespace directly (`Kuru::Info`). Lines are
tagged with your mod's name. See [debugging](../modding/guides/debugging.md).

## Functions

`Info(std::string_view message)`  a normal log line.
`Warning(std::string_view message)`  a warning.
`Error(std::string_view message)`  an error.
`Trace(std::string_view message)`  a low-level trace line, hidden unless the console level includes it.

## Levels

`Kuru::LogLevel` is `Trace`, `Info`, `Warning`, or `Error`. The console filters by level.
