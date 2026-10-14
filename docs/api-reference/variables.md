# Kuru::Variables

Named values (CVars) that persist across restarts and are editable from the console and the Settings tab. See
the [commands and settings guide](../modding/guides/commands-and-cvars.md).

## Functions

### `Register(const char* name, const char* defaultValue, const char* description = "")`

Register a CVar with a default value. Do this in each `OnLoad`. Prefix names with your mod's tag to avoid
clashes.

### `GetString(const char* name) -> std::string`

Read a CVar as a string.

### `GetInt(const char* name) -> int`

Read a CVar as an int.

### `GetFloat(const char* name) -> float`

Read a CVar as a float.

### `GetBool(const char* name) -> bool`

Read a CVar as a bool.

### `Set(const char* name, value)`

Set a CVar. Overloads accept `const char*`, `std::string`, `int`, `float`, and `bool`.

Disabling or reloading a mod preserves its settings. Re-registering a variable restores its latest
value. Use the console's `unset` command or the Settings tab's delete button to remove a saved value.

Missing active variables read as an empty string (`0`/false for numeric/bool getters). `GetBool` recognizes
`1`, `true`, and `True`, other strings are false. Getters convert text without validation.
`Set` saves the current value set, including inactive mods' values, to `%APPDATA%\Kuru\config.cfg`.
Registration alone does not flush the file. An existing active name is not replaced by `Register`.

Register within a host-owned callback. A worker has no current registration context, so a CVar
registered directly there is not tagged for teardown; queue it through Execute. `Set` on an unknown
name creates a persistent ad-hoc variable, removed explicitly with `unset`.

The file stores raw `name=value` lines without escaping. Use names without `=` or newlines and
single-line values. `=` inside a value is preserved. Floats use `std::to_string` formatting.
