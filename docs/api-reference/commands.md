# Kuru::Commands

Register console commands, runnable from the F3 console. See the
[commands guide](../modding/guides/commands-and-cvars.md).

A command handler is `void(const char* args)`, where `args` is the rest of the typed line.

## Functions

`Register(const char* name, const char* help, handler)`
Add a command. `name` is what you type in the console, `help` is the one line shown in the command list, and
`handler` runs when the command is entered.
