# Verifying layouts

Kuru hardcodes a handful of core struct layouts: the `UObject` header and the type hierarchy (`UField`,
`UStruct`, `UClass`, `UFunction`, `UProperty`), the interpreter frame (`FFrame`), and the delegate struct.
These are Rocket League's offsets, not stock UE3's (see
[the fork](../modding/concepts/01-rocket-league-is-a-ue3-fork.md)). Updates can invalidate them.
Reflected members of the game's own classes come from the generated SDK and are handled by
[regenerating](regenerating-the-sdk.md), not here. This page is how to confirm the core offsets are still
right.

## When to suspect a layout change

The symptoms are distinctive. If reflection returns garbage, object names or classes come out as nonsense,
property offsets fall outside an object's size, or hooks read wrong parameters, a core offset probably moved. A
layout break often affects many classes. Wrong globals, stale object pointers, or decoding errors can
produce similar symptoms, so garbage reflection is not a diagnosis by itself.

## Confirming against a known object

The reliable check is to walk an object you already know and confirm the results:

1. Open the Objects tab and search for a class whose shape you know, for example `Core.Object` or a simple
   struct like `Rotator`.
2. Inspect an instance and the class metadata. Its name/class should resolve, and properties should have
   plausible offsets within `PropertiesSize`. The Objects tab walks `Children`, not `PropertyLink`,
   validate that linked list separately if you rely on it. An empty list can also be legitimate.
3. For a function, check that its parameter list matches what you expect and that `ParmsSize` is a small, sane
   number.

Cross-check more than one class and field kind. Plausible output alone can hide an incorrect offset.

## Confirming against the binary

For the frame and other non-reflected layouts, read the actual usage in a disassembler. `ProcessInternal`, for
instance, reads the executing function from `frame + 0x18` and the locals pointer from `frame + 0x30`. If those
change, the accesses in the disassembly change with them; match the loader's offsets to what the code does.
The current `FFrame::PreviousFrame`, `OutParms`, and state-frame tail are explicitly marked inferred in
the SDK. Do not use confirmation of Node/Locals as proof of every adjacent field.

## After a fix

Update the affected `sdk-base` declarations and the separate runtime mirror structures in SDKGen's
`ReflectionWalk.cpp` and `KuruCommon` where applicable. `static_assert` checks that C++ produces the
declared offsets and sizes. It cannot detect changes in the game. Retain current binary or live evidence,
then regenerate and verify the generated classes against known objects again.

## Layout copies to compare

| Area | SDK declarations | Additional readers |
| --- | --- | --- |
| UObject prefix | `sdk-base/Core/ObjectModel.hpp` | `libs/KuruCommon/include/Kuru/Common/Unreal.hpp`: `UObjectPrefix`; SDKGen `ReflectionWalk.cpp` runtime prefix |
| UField/UStruct/UFunction/UProperty | `sdk-base/Core/ObjectModel.hpp` | SDKGen `ReflectionWalk.cpp`: `UFieldRuntime`, `UStructRuntime`, `UFunctionRuntime`, `UPropertyRuntime` |
| FFrame / state-frame | `sdk-base/Core/Script.hpp` | Loader `HookEngine.cpp` frame reads; `DllMain.cpp` interpreter probe |
| Delegate storage | `sdk-base/Core/Delegates.hpp` | SDKGen property-size/type emission and any independently retained engine trace |

A correction in one reader does not automatically update the others. Keep build-specific evidence
for the changed layout and regenerate only after those readers agree.
