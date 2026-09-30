# Rocket League is a UE3 fork

Rocket League runs on Unreal Engine 3 - but not stock UE3. Psyonix forked the engine years ago and changed
it heavily. What they kept and what they changed is the whole basis for how Kuru works.

## What the engine gives us

Because it's UE3, the architecture is known and documented. All of it is present in Rocket League, and Kuru
relies on it:

- **The object system.** Every live thing in the game - actors, components, classes, even functions and
  properties - is a `UObject`, held in a global array (`GObjects`) with names in a global table (`GNames`).
- **Reflection.** Every class describes its own fields and functions at runtime, so you can walk any object
  and discover what it contains without knowing it ahead of time.
- **The UnrealScript VM.** Gameplay logic runs as bytecode through an interpreter, dispatched through a
  handful of well-known entry points (`ProcessEvent` and its siblings).

Knowing UE3 tells us *what to look for* - the shapes and the mechanisms. That part doesn't change.

## What the fork changes

The fork changes the details - and it changes enough that stock UE3 knowledge, or an SDK ripped from another
UE3 game, is simply wrong for Rocket League:

- **Struct layouts.** Field offsets are shifted. The interpreter's stack frame, for instance, has every
  field moved 16 bytes from where stock UE3 puts them. A script delegate is 24 bytes where stock UE3 uses 16.
- **Bytecode.** The opcode byte-values the VM uses differ from stock, so you can't decode Rocket League
  bytecode with a stock UE3 table.
- **Added and repurposed pieces.** Psyonix added their own fields and reused spare flag bits for their own
  meaning - a flag that means "editor-only" in stock UE3 marks a networked delegate in Rocket League.

None of this is documented. Every offset, every opcode, every flag has to be recovered from the actual game,
and re-checked when the game changes it.

## Why this shapes Kuru

That split - known architecture, unknown details - is why Kuru is built the way it is:

- It **generates its SDK from the live game**, so field offsets come from the game's own reflection data
  instead of from assumptions. When an update moves offsets, regenerating the SDK fixes it.
- It **resolves the engine's internals at runtime** by pattern and behavior, not hardcoded addresses, so an
  update that shifts code around doesn't break the loader.
- It **verifies everything against the real binary.** A layout that looks right in stock UE3 is a guess
  until it's confirmed in Rocket League.

The rest of these pages take each mechanism in turn - the [object system](02-the-object-system.md), the
[VM](04-the-unrealscript-vm.md), and how Kuru [boots](06-how-kuru-boots.md) on top of them.
