# Commands and settings

## Console commands

Register a command in `OnLoad` and it's available in the F3 console:

```cpp
Kuru::Commands::Register("greet", "greet - say hello", [](const char* args) {
    Kuru::Info(std::string("hello ") + (args && *args ? args : "there"));
});
```

The callback gets the rest of the line as `args`. Type `greet` (or `greet world`) in the console to run it.

## Settings (CVars)

CVars are named values that persist across restarts and are editable from the console and the Settings tab.
Register once, then read them anywhere:

```cpp
Kuru::Variables::Register("mymod_speed", "1.0", "movement multiplier");

float speed = Kuru::Variables::GetFloat("mymod_speed");
```

Read with `GetFloat` / `GetInt` / `GetBool` / `GetString`, and write with `Kuru::Variables::Set(name, value)`.
From the console: `set mymod_speed 2.0`, `get mymod_speed`.

Prefix your cvar names (`mymod_`) so they don't collide with other mods.
