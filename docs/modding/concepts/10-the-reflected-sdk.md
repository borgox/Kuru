# The reflected SDK

Reflection (see [reflection and game state](03-reflection-and-game-state.md)) lets you read any field on any
object by walking its type at runtime. That's powerful but slow and verbose - you don't want a `PropertyLink`
loop every time you read the ball's velocity. The reflected SDK turns that runtime walk into plain typed C++,
generated once.

## Two halves: sdk-base and generated-sdk

The SDK a mod includes is in two parts:

- **`sdk-base`** - hand-authored, runtime-verified declarations of the engine core: `UObject` and the type
  hierarchy, `FName`, the containers, the reflection machinery, and the mod-facing API. This is the stable part;
  it changes when the *engine's* layout does, which is rare.
- **`generated-sdk`** - the Rocket League classes (`ACar_TA`, `ABall_TA`, and thousands more), generated from
  the game's own reflection, with every field at the offset reflection reported. This is the volatile part; it
  changes whenever an update moves offsets.

A mod pulls in both through the one umbrella header, so you rarely think about the split.

## How it's generated

`KuruSDKGen`, injected into a running game, does it in one pass: it walks `GObjects`, reads every
class's properties and functions (names, kinds, offsets, flags), and emits the typed headers: a struct per
class with fields at their offsets, a params struct per function, enums, and so on.

The result is `generated-sdk`, the runtime reflection frozen into code. The Objects tab and the generated
headers are, again, two views of the same data.

## Why it's version-specific

Because offsets are baked into the generated structs, the SDK is only correct for the game version it was
generated from. When Rocket League updates and offsets shift, `car->SomeField` would read the wrong bytes - so
you regenerate the SDK against the new version. The loader itself doesn't need regenerating (it resolves the
engine at runtime; see [how Kuru boots](06-how-kuru-boots.md)); the *SDK* does, and that's the main thing an
update asks of you.

The [maintaining](../../maintaining) docs cover regenerating the SDK and confirming it's correct against a new
game build.
