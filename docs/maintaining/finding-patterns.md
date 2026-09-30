# Finding patterns

Kuru finds the game's globals and entry points at runtime instead of hardcoding addresses (see
[how Kuru boots](../modding/concepts/06-how-kuru-boots.md)). Most of that survives updates on its own. The one
part that can break is a byte pattern: when Psyonix rewrites the code around a global, the signature Kuru scans
for stops matching and that resolver comes back empty. This page is how to fix one.

## What resolves how

Not everything is a pattern, and the non-pattern parts rarely need attention:

- **Vtable layers** (`ProcessEvent` and the others) are read from a live object's vtable by index. These only
  break if Psyonix reorders the vtable, which is rare.
- **`ProcessInternal`** is read from a script function's `Func` pointer. Structural, not a pattern.
- **`GNatives`** is found by scanning `ProcessInternal` for the table access and validating that the candidate
  is 256 code pointers. The validation means a wrong match is rejected rather than used.
- **`GNames` and `GObjects`** are found by a byte pattern that references them. This is the one most likely to
  need updating.
- **The interpreter globals** are found from a `GNatives` handler's writes, then confirmed by an active probe.

So in practice "a pattern broke" almost always means the `GNames`/`GObjects` signature.

## Recognising a broken pattern

The bootstrap log says which step failed. A line like `global bootstrap failed: GNames/GObjects not resolved`,
or a resolver reporting zero, is the signal. If bootstrap does not get past that, the loader does nothing.

## Re-deriving the signature

Open the current Rocket League binary in a disassembler (IDA or similar):

1. Find the global. `GObjects` is a large array of pointers, and `GNames` sits near it. Cross-references from a
   function you can identify (anything that iterates all objects, or resolves a name) lead to them.
2. Pick a short, unique instruction sequence that references the global and turn it into a signature: the fixed
   opcode bytes, with wildcards over the address operand.
3. Confirm the signature matches exactly once in the module, and that the operand resolves to the global.

Update the pattern in the loader's bootstrap and rebuild. Because the resolvers downstream (`GNatives`, the
interpreter globals) validate what they find, a correct `GNames`/`GObjects` unfolds the rest.

## Why this stays rare

Patterns are chosen to be stable: distinctive instruction shapes rather than exact addresses. An update that
just relocates code leaves them intact. Only a rewrite of the code around a global forces a new signature.
