# The object system

Everything in Rocket League that game logic can touch is a `UObject`: actors, components, the ball, your
car - and also the *types* themselves. Every class, function, and property is a `UObject` too. This is the
core of UE3's reflection, and it's what lets Kuru read the game without knowing its layout ahead of time.

All offsets on this page are Rocket League's, recovered from the game and verified against live objects.
They are not stock UE3 offsets (see [the fork](01-rocket-league-is-a-ue3-fork.md)).

## Names: FName and GNames

Names aren't stored as strings on each object. UE3 interns every name once into a global table, `GNames`,
and refers to it by index. An `FName` is 8 bytes - an index into `GNames`, plus a `Number` that
disambiguates duplicates (`Foo`, `Foo_1`, ...):

```
struct FName { int32 Index; int32 Number; };
```

Turning an `FName` into text is a lookup: `GNames[Index]`. Comparing two names is comparing two integers, no
string work. Kuru resolves `GNames` at startup so it can print and match names.

## The object header

Every `UObject` begins with the same header:

```
0x00  VfTableObject           C++ vtable (also how Kuru finds ProcessEvent)
0x10  ObjectFlags             RF_* flags
0x28  Linker
0x38  ObjectInternalInteger   this object's index in GObjects
0x40  Outer                   the object that contains this one
0x48  Name                    FName
0x50  Class                   the UClass this is an instance of
0x58  ObjectArchetype
```

`Class` is the field everything hinges on: it points at the object's type, and the type describes the
object's layout.

## GObjects

`GObjects` is a global array of every live `UObject` - typically 180,000+. Kuru resolves
it at startup, and the **Objects tab is just a search over it**. Iterating `GObjects` and checking each
object's `Class` is how you find "every car" or "every function named `OnHitGoal`".

## The type hierarchy

Types are objects too, and they form a small hierarchy - each level adds fields:

```
UObject
└─ UField            a member of a type; adds Next (0x60), linking siblings
   └─ UStruct        a type with members: SuperStruct, Children, Script, PropertyLink
      ├─ UClass      a runtime class you can instantiate
      ├─ UFunction   a callable: flags, params, and bytecode or a native pointer
      └─ UState

UField
└─ UProperty         one field: its offset, size, kind, and flags
   ├─ FloatProperty · IntProperty · BoolProperty · ByteProperty
   ├─ ObjectProperty · StructProperty · ArrayProperty · NameProperty
   └─ ...
```

A `UProperty`'s *own* `Class` tells you what kind it is: a property object whose class is `FloatProperty`
describes a `float` field; one whose class is `StructProperty` describes an embedded struct; and so on.

## How a type describes its instances

A `UStruct` (and therefore a `UClass`) holds two things that matter most:

- **`SuperStruct` (0x80)** - its parent type. Walking `SuperStruct` gives the inheritance chain
  (`Car_TA → Vehicle_TA → Pawn → Actor → Object`), which is how `IsA` works.
- **`PropertyLink` (0x110)** - the head of a linked list of every property in the type, inherited ones
  included, chained through `PropertyLinkNext`. This is the fast path for "walk all fields".

`Children` (0x88) is a broader list of *every* member - properties, functions, enums - chained through
`UField::Next`; `PropertyLink` is the properties-only shortcut.

Each `UProperty` says where its field lives and how big it is:

```
0x74  ElementSize        bytes for one element
0x78  PropertyFlags      CPF_* (parameter, out-param, return, ...)
0x98  Offset             where this field sits inside an instance
0xA0  PropertyLinkNext
```

## Reflection in practice

Put it together and reading a field by name on any object is mechanical:

```cpp
// read the float property "FOV" on some object
UClass* cls = object->Class;
for (UProperty* p = cls->PropertyLink; p; p = p->PropertyLinkNext) {
    if (p->Name == "FOV") {
        float value = *reinterpret_cast<float*>(
            reinterpret_cast<uint8_t*>(object) + p->Offset);
        // ...
    }
}
```

You never hardcode `FOV`'s offset - you ask the type. This is exactly what the Objects tab does when it lists
a property, what the `Game::` helpers do under the hood, and what the SDK generator bakes into typed structs
so a mod can skip the walk and just write `object->FOV`.

## Functions and parameters

A `UFunction` is a `UStruct` whose `Children` are its parameters (each a `UProperty` flagged `CPF_Parm`)
followed by its locals. Its own fields describe how it runs:

```
0x130 FunctionFlags     FUNC_Native, FUNC_Event, ...
0x138 iNative           native index, if any
0x146 ParmsSize         total size of the parameter block
0x158 Func              native C++ implementation, when native
```

That parameter description is what lets Kuru hand a hook a typed view of a call's arguments - the
[VM](04-the-unrealscript-vm.md) and [hook engine](07-the-hook-engine.md) pages build directly on it.

## Why the offsets are exact here

These offsets are read from Rocket League and confirmed against live objects, not assumed from stock UE3.
When an update shifts them, the SDK generator re-reads them from the game's own reflection - which is the
whole reason Kuru generates its SDK instead of hardcoding it. The next page,
[reflection and game state](03-reflection-and-game-state.md), follows that from the raw fields up to reading
the ball's speed.
