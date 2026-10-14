# Kuru documentation

Use these guides to install Kuru, write mods, or maintain the loader and SDK. Source links open files
in this checkout or on GitHub.

## Using Kuru

- [Installation, paths, updates, and eject](installing.md)
- [Building Kuru from source](building.md)
- [Troubleshooting](troubleshooting.md)

## Writing a mod

Start here in order:

1. [What is a mod?](modding/getting-started/what-is-a-mod.md)
2. [Generate the standalone template and set up the compiler](modding/getting-started/environment-setup.md)
3. [Your first mod](modding/getting-started/your-first-mod.md)
4. [Build and load it](modding/getting-started/building-and-loading.md)

### Guides

- [Hooks](modding/guides/hooks.md)
- [Parameters](modding/guides/editing-parameters.md)
- [Game state](modding/guides/reading-game-state.md)
- [Events](modding/guides/events.md)
- [GUI/input](modding/guides/gui.md)
- [Threading](modding/guides/threading.md)
- [Commands/CVars](modding/guides/commands-and-cvars.md)
- [Debugging](modding/guides/debugging.md)
- [Distribution](modding/guides/distributing-your-mod.md)

## API reference

- [Typed SDK](api-reference/sdk.md)
- [Mod lifecycle/API versions](api-reference/mod.md)
- [Hooks/context](api-reference/hooks.md)
- [Events](api-reference/events.md)
- [Tasks](api-reference/thread.md)
- [GUI](api-reference/gui.md)
- [Game helpers](api-reference/game.md)
- [Commands/eject](api-reference/commands.md)
- [CVars](api-reference/variables.md)
- [Logging](api-reference/log.md)
- [Storage](api-reference/storage.md)

## Engine concepts

- [Rocket League's fork](modding/concepts/01-rocket-league-is-a-ue3-fork.md)
- [Objects](modding/concepts/02-the-object-system.md)
- [Reflection](modding/concepts/03-reflection-and-game-state.md)
- [VM](modding/concepts/04-the-unrealscript-vm.md)
- [Dispatch layers](modding/concepts/05-function-dispatch-layers.md)
- [Bootstrap](modding/concepts/06-how-kuru-boots.md)
- [Hook implementation](modding/concepts/07-the-hook-engine.md)
- [Argument internals](modding/concepts/08-editing-parameters-internals.md)
- [Delegates](modding/concepts/09-delegates.md)
- [Generated SDK](modding/concepts/10-the-reflected-sdk.md)

## Maintaining Kuru

- [Game updates](maintaining/updating-for-a-new-version.md)
- [SDK regeneration](maintaining/regenerating-the-sdk.md)
- [Patterns](maintaining/finding-patterns.md)
- [Layout verification](maintaining/verifying-layouts.md)
- [SDK verification annotations](../sdk-base/Core/Script.hpp)

- [Future tasks](future-tasks.md)

## Terms used in these docs

| Term | Meaning |
| --- | --- |
| UE3 | Unreal Engine 3. Rocket League uses a modified version. |
| SDK | Software development kit. Kuru combines handwritten core types with generated game headers. |
| DLL | Windows dynamic-link library. Kuru loads each compiled mod as a DLL inside the game process. |
| CVar | Console variable: a named setting accessible through the console and Settings tab. |
| GUI / UI | Graphical user interface / user interface. Kuru draws its overlay with Dear ImGui. |
| VM | Virtual machine. The UnrealScript interpreter executes game bytecode. |
| ABI | Application binary interface: calling conventions, layouts, and other rules compiled code must agree on. |
| RVA | Relative virtual address: an address expressed as an offset from a loaded module's base. |
| SEH | Windows structured exception handling, used by some Kuru callback guards to catch access violations. |
| RPC | Remote procedure call: a function invocation sent over the network. |
| Borrowed pointer | A pointer to storage owned elsewhere. Copying it does not keep the object alive. |

> Big part of the documentation has been written by AI, if you're not a fan of it then figure everything out yourself
See [threading](modding/guides/threading.md) for dispatcher, window thread, and bootstrap worker.
