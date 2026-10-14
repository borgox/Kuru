# Typed game SDK

Include `<Kuru/Kuru.hpp>` from the generated standalone template. It includes the host-facing API,
handwritten core SDK, and generated game types. The generated `Kuru/Api.hpp` also imports convenience
using-directives; explicit namespaces make examples easier to follow.

| Namespace | Contents |
| --- | --- |
| `Kuru` | `IMod`, logging, tasks, and host API wrappers |
| `Kuru::SDK` | Handwritten `UObject`, reflection types, math, names, strings, and containers |
| `Kuru::RL` | Generated reflected classes, structs, enums, and nested parameter structures |

Generated classes extend the handwritten core where applicable. `HookContext::Caller()` returns a
core `UObject*`; it is not automatically a ball or car. Check its type before casting.

## Objects and lookup

| API | Use |
| --- | --- |
| `UObject::FindClass(name)` | Find a reflected class by name; returns null when missing |
| `UObject::FindFunction(name)` | Find a reflected function, such as `Function TAGame.Ball_TA.OnHitGoal` |
| `UObject::FindObject<T>(name)` | Look up with T::StaticClass() as the required class; returns null when no match is found |
| `object->GetName()` / `GetPathName()` | Short name or outer-qualified path as `std::string` |
| `object->IsA(T::StaticClass())` | Check the runtime class and superclass chain |
| `object->IsTemplate()` | Check class-default/archetype flags |

Lookups search the current global array; a class or instance not yet loaded cannot be found. Returned
pointers belong to the game. Do not delete them, keep them across map transitions, or assume a lookup
pins their lifetime. `Game::IsValid` checks array identity, not all destruction state. Do game-object
work on the dispatcher and only access a hook's caller for the callback's lifetime.

## Reading a typed field

This example reads the ball's velocity magnitude from an update callback:

```cpp
void OnUpdate(float) override {
    if (auto* ball = Kuru::Game::GetBall()) {
        const auto& velocity = ball->Velocity;
        const double speed = std::sqrt(
            static_cast<double>(velocity.X) * velocity.X +
            static_cast<double>(velocity.Y) * velocity.Y +
            static_cast<double>(velocity.Z) * velocity.Z);
        // Use speed here, or publish a synchronized snapshot for OnRender.
        (void)speed;
    }
}
```

Add `<cmath>` and put the callback in an `IMod` subclass. This is a compile-checked field-read example,
not an independent in-game validation of velocity units. `GetBall` can fall back to a global scan.
Use generated headers matching your Rocket League build; wrong layouts can read unrelated bytes.

## Arrays, names, and strings

`TArray<T>` stores Data/Count/Max. Use `Num`, `IsValidIndex`, indexing, or range-for to read it.
Indexing is unchecked. Treat arrays embedded in game objects as borrowed views; copying a TArray
does not deep-copy the engine's buffer. Do not call mutation/free helpers on engine-owned arrays
without verifying their allocation and element-lifetime requirements.

`FName` stores an index and number. `ToString()` returns text, while `GetIndex()`/`GetNumber()` expose
the components. Prefer an existing object's `GetName`/`GetFName` to constructing names. Native name
construction and string/container ownership still have unresolved SDK limitations.

`FString` is wide-character array storage. `View()` borrows its characters; `ToWString()` and `ToAnsi()`
produce standard strings. Default copies share the underlying buffer. A view is valid only while that
engine buffer is valid. Use standard strings for mod-owned UI snapshots.

## Generated methods and parameter structures

Open `generated-sdk/Generated/Packages/<package>.hpp` and find the class and method. Generated
parameter structs are nested in their declaring class and named `<Method>_Params` (with sanitized names
where needed). Inspect their fields and padding before using them in `ctx.Params<T>()`.

Generated convenience methods call through ProcessEvent and cache the reflected function pointer.
They are real game calls, not just field reads. A handwritten helper with a native-looking name is
not necessarily implemented: check its body/runtime binding before using it. For example, the current
loader does not supply the delegate invocation binding.

For packed 4-byte bool properties, the generator emits `Get<Field>()`/`Set<Field>(bool)` accessors
that preserve unrelated bits. Follow the generated declaration and metadata rather than assuming a
standalone C++ bool. General bool-mask semantics remain marked for verification in the core header.

Sources: [ObjectModel.hpp](../../sdk-base/Core/ObjectModel.hpp),
[Containers.hpp](../../sdk-base/Core/Containers.hpp), [Strings.hpp](../../sdk-base/Core/Strings.hpp),
and [Generator.cpp](../../libs/KuruSDKGen/src/Generator.cpp).
