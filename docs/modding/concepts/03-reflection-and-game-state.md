# Reflection and game state

The [object system](02-the-object-system.md) page showed that a type is a list of `UProperty` objects, each
with an `Offset` into the instance and a `Class` naming its kind. This page turns that into actually reading
values - and into the typed SDK you use from a mod.

## Property kinds

A property's `Class` tells you its kind. The kind's subclass adds one specializing field at `0xC8`. The
common kinds and how each is stored:

| Kind | Stored as | Read it |
| --- | --- | --- |
| `FloatProperty` | `float` (4 bytes) | `*(float*)(base + Offset)` |
| `IntProperty` | `int32` (4) | `*(int32*)(base + Offset)` |
| `QWordProperty` | `uint64` (8) | `*(uint64*)(base + Offset)` |
| `ByteProperty` | `uint8` (1) | `*(uint8*)(base + Offset)`; if `Enum` is set it's an enum value |
| `BoolProperty` | a bit in a `uint32` | `(*(uint32*)(base + Offset) & BitMask) != 0` |
| `ObjectProperty` | `UObject*` (8) | `*(UObject**)(base + Offset)`; `PropertyClass` is the expected type |
| `NameProperty` | `FName` (8) | look up in `GNames` |
| `StrProperty` | `FString` (16) | a `TArray` of wide chars |
| `StructProperty` | embedded struct | recurse into `Struct`'s properties at `base + Offset` |
| `ArrayProperty` | `TArray` (16) | `Inner` describes the element; walk `Count` items from `Data` |

### The bool trap

`BoolProperty` is the one that bites people. A UE3 bool isn't a byte - it's a single bit packed into a
`uint32`, selected by the property's `BitMask` (at `0xC8`). Reading the raw byte gives garbage; you have to
mask:

```cpp
uint32 word = *reinterpret_cast<uint32*>(base + prop->Offset);
bool value = (word & boolProp->BitMask) != 0;
```

## Walking every field

Because `PropertyLink` chains all of a type's properties - inherited ones included - enumerating an object's
fields is a loop:

```cpp
for (UProperty* p = object->Class->PropertyLink; p; p = p->PropertyLinkNext) {
    const uint8_t* value = reinterpret_cast<uint8_t*>(object) + p->Offset;
    // decode by p->Class kind, using the table above
}
```

That loop is exactly what the Objects tab runs to list a selected object's properties.

## IsA and casting

Types carry `SuperStruct` (their parent), so asking whether an object is a kind of something is a walk up the
chain:

```cpp
bool IsA(UObject* o, UClass* type) {
    for (UStruct* c = o->Class; c; c = c->SuperStruct)
        if (c == type) return true;
    return false;
}
```

Casting is the same check: confirm the object's class chain contains the target type, then reinterpret the
pointer.

## Finding objects

To find objects you scan `GObjects`. "Find the class `Car_TA`" or "find every live `Ball_TA`" is a filter over
the global array by name or by `IsA`. Kuru wraps the common lookups (`FindObject`, `FindClass`), and the
`Game::` helpers are thin shortcuts on top of them.

## From reflection to a typed SDK

Doing this walk at runtime for every field would be slow and verbose, so the SDK generator does it **once**:
it reads every class's `PropertyLink`, records each property's name, kind, and offset, and writes out typed
C++ structs. A generated `ACar_TA` has its fields at the offsets reflection reported, so a mod skips the walk
and writes `car->SomeField` directly - the offset is baked in.

Two things follow from that:

- The typed SDK is only correct for the game version it was generated from. When offsets move in an update,
  you regenerate it - see [the reflected SDK](10-the-reflected-sdk.md).
- The Objects tab and the generated SDK are two views of the same data: one walks it live, the other freezes
  it into headers.

The [reading game state](../guides/reading-game-state.md) guide sits at the top of all this - `Game::GetBall()`
finds the object and the typed SDK reads its fields, every bit of it grounded in the reflection described here.
