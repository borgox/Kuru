# Verifying layouts

Kuru hardcodes a handful of core struct layouts: the `UObject` header and the type hierarchy (`UField`,
`UStruct`, `UClass`, `UFunction`, `UProperty`), the interpreter frame (`FFrame`), and the delegate struct.
These are Rocket League's offsets, not stock UE3's (see
[the fork](../modding/concepts/01-rocket-league-is-a-ue3-fork.md)), and they rarely move, but a large update
can shift them. Everything else (the game's own classes) comes from the generated SDK and is handled by
[regenerating](regenerating-the-sdk.md), not here. This page is how to confirm the core offsets are still
right.

## When to suspect a layout change

The symptoms are distinctive. If reflection returns garbage, object names or classes come out as nonsense,
property offsets fall outside an object's size, or hooks read wrong parameters, a core offset probably moved. A
layout break shows up everywhere at once, unlike a single wrong field.

## Confirming against a known object

The reliable check is to walk an object you already know and confirm the results:

1. Open the Objects tab and search for a class whose shape you know, for example `Core.Object` or a simple
   struct like `Rotator`.
2. Inspect an instance. Its `Name` and `Class` should resolve to sane text, and its properties should list
   with plausible names and offsets. If the property list is empty or garbage, a link offset (`Children` or
   `PropertyLink` on `UStruct`) is wrong.
3. For a function, check that its parameter list matches what you expect and that `ParmsSize` is a small, sane
   number.

Because these are real objects with known contents, a wrong offset produces obviously wrong output.

## Confirming against the binary

For the frame and other non-reflected layouts, read the actual usage in a disassembler. `ProcessInternal`, for
instance, reads the executing function from `frame + 0x18` and the locals pointer from `frame + 0x30`. If those
change, the accesses in the disassembly change with them; match the loader's offsets to what the code does.

## After a fix

Update the offsets in `sdk-base`. They are asserted with `static_assert`, so a mismatch against a known size
fails the build. Then regenerate the SDK so the generated classes sit on top of the corrected core, and verify
once more against a known object.
