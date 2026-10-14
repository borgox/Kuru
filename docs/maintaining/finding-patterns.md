# Finding patterns

Kuru resolves many game globals and entry points at runtime (see
[how Kuru boots](../modding/concepts/06-how-kuru-boots.md)). Changed data or instructions can invalidate
a pattern. Fixed layouts, vtable slots, argument
handling, and fallback addresses must be checked separately. This page covers resolver failures.

## What resolves how

The current resolution paths are:

- **Vtable layers** (`ProcessEvent` at 67, `CallFunction` at 76) are read from a live object's table.
  Correct indices alone do not prove that a function's behavior or calling shape is unchanged.
- **`ProcessInternal`** is read from a script function's `Func` pointer. Structural, not a pattern.
- **`GNatives`** is found by scanning `ProcessInternal` for candidate table references and checking the
  first 256 pointers against the executable section. Also verify what those functions do.
- **`GNames`** matches an in-image data signature. The match address is used directly as the name-array
  header, not decoded as a reference from an instruction.
- **`GObjects`** is derived at `GNames + 0x48`. Both headers and sampled object/name relationships are
  validated. The displacement is a version-specific assumption.
- **`GMalloc`** uses a separate RIP-relative instruction pattern. The loader requires a usable allocator before loading mods.
- **The interpreter globals** are inferred from handler writes and an active probe. Runtime failure
  falls back to build-specific RVAs in the loader. A fallback log line is a reason to validate those
  addresses, not evidence that the resolver succeeded.

SDKGen additionally resolves optional build metadata. Not every missing metadata pattern is fatal.

## Recognising a broken pattern

The bootstrap log says which step failed. A line like `global bootstrap failed: GNames/GObjects not resolved`,
or `game allocator unavailable` identifies a required dependency that failed. Missing GNatives can
leave native argument reading unavailable while other hooks still install. Diagnose the particular
failure rather than assuming every warning has the same cause.

## Re-deriving the signature

Open the current Rocket League binary in a disassembler (IDA or similar):

For GNames, locate the live array header and verify Data/Count/Max and its name entries. Check the
object-array header at `+0x48`. Compare the header bytes with `GNamesPattern`, keeping relocated
pointers wildcarded, and verify the candidate with the globals validator. The scanner searches the
module image, including data; this path does not decode an instruction operand.

GMalloc uses a different mechanism: a RIP-relative instruction signature. Locate its initialization
sequence, wildcard the displacement, and verify the pointer slot it references. The current loader
reads a signed displacement at match `+3`, relative to the next instruction at `+7`.

Update the appropriate source and rebuild. GNames/GObjects resolution is shared in `KuruCommon`;
allocator/metadata pattern strings also appear in the loader or SDKGen. Check each affected consumer.
Then verify downstream resolution, reflection, hooks, and parameter behavior independently.

## Evidence to keep

Record the game build, matched data header or instruction, decoded address where applicable, and
validation result. Keep the disassembly/trace used to establish meaning. A relocated address can still match
the same signature. After an update, verify that the match still identifies the intended data or function.

## Source locations

| Binding | Files and symbols |
| --- | --- |
| GNames signature | `libs/KuruCommon/include/Kuru/Common/Pattern.hpp`: `GNamesPattern` |
| Match used directly as GNames | `libs/KuruCommon/src/Globals.cpp`: `TryInitializeRocketLeagueGlobalsFromPattern` |
| GObjects `+0x48` and validation | `libs/KuruCommon/src/Unreal.cpp`: `GObjectsFromGNamesOffset`, `ValidateRocketLeagueGlobalsFromGNames` |
| GMalloc instruction pattern | `libs/KuruLoader/src/DllMain.cpp`: `BootstrapGlobals`; separate copy in SDKGen's `ReflectionWalk.cpp` |
| Interpreter globals / fallback RVAs | `DllMain.cpp`: `ResolveInterpreterGlobals`, `BootstrapGlobals` |
