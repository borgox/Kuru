# The reflected SDK

Reflection (see [reflection and game state](03-reflection-and-game-state.md)) lets you read any field on any
object by walking its type at runtime. That's powerful but slow and verbose - you don't want a `PropertyLink`
loop every time you read the ball's velocity. The reflected SDK turns that runtime walk into plain typed C++,
generated once.

## Two halves: sdk-base and generated-sdk

The SDK a mod includes is in two parts:

- **`sdk-base`** - hand-authored declarations of the engine core: `UObject` and the type
  hierarchy, `FName`, the containers, the reflection machinery, and the mod-facing API. These handwritten
  layouts and APIs require separate maintenance. Some members are verified and others inferred or
  partially implemented; see [SDK verification annotations](../../../sdk-base/Core/Script.hpp).
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

Dynamic arrays use the reflected element type. For example, a product array is generated as
`Kuru::SDK::TArray<Kuru::RL::UProduct_TA*>`, so indexing it returns a product pointer without a cast.
This also applies to function parameters and return values. Unsupported element types retain their
reflected byte size as opaque storage; they do not gain typed access to unverified internals.

The current generator also writes a self-contained `mod-template` folder: SDK runtime and mod-facing
headers, template code/build files, matching ImGui core sources, licenses, and a copy of the fresh game
headers. The nongenerated files are embedded in `KuruSDKGen.dll` at build time, not read from the
developer's source checkout at runtime. Mod builds use only that folder and the local C++ toolchain.

## Why it's version-specific

Because offsets are baked into the generated structs, the SDK is only correct for the game version it was
generated from. When Rocket League updates and offsets shift, `car->SomeField` would read the wrong bytes - so
you regenerate the SDK against the new version. The loader does not generate itself, but it also compiles
typed cursor helpers against generated game headers and relies on fixed core layouts and bindings.
Check it against the new build too; successful SDK generation does not prove every loader path is compatible.

The [maintaining](../../README.md) docs cover regenerating the SDK and confirming it's correct against a new
game build.
