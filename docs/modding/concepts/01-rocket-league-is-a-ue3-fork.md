# Rocket League is a UE3 fork

Rocket League runs on Unreal Engine 3 - but not stock UE3. Psyonix forked the engine years ago and changed
it heavily. What they kept and what they changed is the whole basis for how Kuru works.

## What the engine gives us

UE3 gives us an architectural starting point. Kuru relies on these mechanisms in Rocket League:

- **The object system.** Reflected game entities - actors, components, classes, even functions and
  properties - are `UObject`s, held in a global array (`GObjects`) with names in a global table (`GNames`).
- **Reflection.** Reflected classes describe exposed fields and functions at runtime. Unreflected native
  fields and C++ implementation details still require separate layout or binary analysis.
- **The UnrealScript VM.** Gameplay logic runs as bytecode through an interpreter, dispatched through a
  handful of well-known entry points (`ProcessEvent` and its siblings).

Knowing UE3 tells us what to look for; it does not establish a Rocket League layout or calling convention.

## What the fork changes

The fork changes the details - and it changes enough that stock UE3 knowledge, or an SDK ripped from another
UE3 game, is simply wrong for Rocket League:

- **Struct layouts.** The recovered `FFrame` fields Node/Object/Code/Locals sit at `0x18/0x20/0x28/0x30`.
  The SDK comments identify these four as disassembly-confirmed; later fields are inferred. The SDK
  models a single `FScriptDelegate` as 16 bytes and the legacy property layout as 24 bytes. Current
  engine delegate properties report 32 bytes and are kept opaque; see [delegates](09-delegates.md).
- **Bytecode.** The SDK deliberately omits a stock `EX_*` opcode-value table. Earlier fork research
  reports differences; decode through verified current handlers rather than assuming a stock table.
- **Added and repurposed pieces.** Psyonix-specific fields and possible flag reinterpretations must be
  checked individually. The proposed NetDelegate meaning of `0x20000000` remains a research claim;
  the current flags header still names that bit `FUNC_EditorOnly`.

Use the [SDK verification annotations](../../../sdk-base/Core/Script.hpp) to distinguish current code, source comments
recording earlier verification, inferred layouts, and layout assumptions.

## Why this shapes Kuru

That split - known architecture, unknown details - is why Kuru is built the way it is:

- It **generates its SDK from the live game**, so field offsets come from the game's own reflection data
  instead of from assumptions. Regeneration helps with reflected offsets only if the core layout used
  to read reflection is still correct.
- It **resolves many engine bindings at runtime** by patterns and structural checks. Fixed vtable indices,
  core layouts, and the interpreter-global RVA fallback can still require an update.
- It **needs Rocket League-specific verification.** A declaration or compile-time size assertion is not
  proof that the same bytes have that meaning in the current game build.

The rest of these pages take each mechanism in turn - the [object system](02-the-object-system.md), the
[VM](04-the-unrealscript-vm.md), and how Kuru [boots](06-how-kuru-boots.md) on top of them.
