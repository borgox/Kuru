# Kuru::Variables

Named values (CVars) that persist across restarts and are editable from the console and the Settings tab. See
the [commands and settings guide](../modding/guides/commands-and-cvars.md).

## Functions

`Register(const char* name, const char* defaultValue, const char* description = "")`
Register a CVar with a default value. Do this once, in `OnLoad`. Prefix names with your mod's tag to avoid
clashes.

`GetString(const char* name) -> std::string`
Read a CVar as a string.

`GetInt(const char* name) -> int`
Read a CVar as an int.

`GetFloat(const char* name) -> float`
Read a CVar as a float.

`GetBool(const char* name) -> bool`
Read a CVar as a bool.

`Set(const char* name, value)`
Set a CVar. Overloads accept `const char*`, `std::string`, `int`, `float`, and `bool`.
