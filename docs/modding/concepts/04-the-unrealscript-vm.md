# The UnrealScript VM

Rocket League exposes both UnrealScript bytecode and native C++ functions through reflection.
The repository does not establish what proportion of gameplay runs in either form. How both run, and where a call's
parameters live, is what the hook system is built on.

## Two kinds of function

Every `UFunction` is one of two things:

- **Script.** Its body is bytecode, stored in `UFunction::Script`, and it runs through the interpreter.
- **Native.** Its body is C++. `UFunction::Func` points at the implementation and it's marked `FUNC_Native`.
  Some natives are also *indexed* (`iNative` set) - the engine's core operators, dispatched by number rather
  than by pointer.

The same `UFunction` machinery describes both; only how the body runs differs.

## Bytecode and GNatives

Script bytecode is a stream of **opcodes**. Each opcode is one byte that selects a handler, followed by that
handler's operands - which are themselves expressions, so bytecode nests.

Kuru resolves **`GNatives`**, the opcode-handler table, and validates its first 256 pointer entries
for byte-valued opcode dispatch. This does not establish the full table's size for indexed natives:

```cpp
// roughly
void Handler(UObject* context, FFrame& stack, void* result);
GNatives[opcode](context, stack, result);
```

A handler reads its operands from the frame's instruction pointer, advances that pointer, and writes the
expression's value into `result`. Evaluating an expression is: read the opcode byte, call `GNatives[opcode]`,
let it consume its operands and produce a value. Running a function is doing that in a loop until a return.

Kuru resolves `GNatives` at startup and *borrows* it - to read a native call's arguments it asks the game's
own handlers to evaluate them, so this reconstruction does not need a handwritten opcode-meaning table.
It still depends on the correct table address and calling shape; it does not validate a stock opcode map
(see [the fork](01-rocket-league-is-a-ue3-fork.md)).

## The frame

A running script function has an `FFrame`. The current SDK declares:

```
0x18  Node            the UStruct / UFunction being executed
0x20  Object          the object it runs on
0x28  Code            the instruction pointer into the bytecode
0x30  Locals          the parameter + local-variable buffer
0x38  PreviousFrame  inferred; not independently disassembly-verified
0x40  OutParms       inferred; not independently disassembly-verified
```

`Code` and `Locals` are the two that matter for hooking: `Code` is where the interpreter is reading from, and
`Locals` is where a script function's parameters and locals live.

`Core/Script.hpp` records Node/Object/Code/Locals as disassembly-confirmed. Its later frame/state-frame
fields include inferred layouts. Compile-time size checks enforce these declarations, not the game's
runtime semantics. See [SDK verification annotations](../../../sdk-base/Core/Script.hpp).

## Where parameters live

This is the crux, and it depends on *how* a function is invoked:

- A **script function** invoked through the event path gets a fresh frame, and its parameters are copied into
  that frame's `Locals`. So the parameters are a real buffer you can point a struct at.
- A **native function** invoked inline from bytecode has no params buffer. It reads its arguments straight
  from the *caller's* bytecode as it runs - one `GNatives` evaluation per argument. There's nothing to point a
  struct at until you reconstruct it.

That difference is the whole reason a name-based hook has to fire at the layer where the parameters actually
exist. The [dispatch layers](05-function-dispatch-layers.md) page maps out those layers, and
[editing parameters, internals](08-editing-parameters-internals.md) covers how Kuru reconstructs a native
call's arguments so you can read and edit them anyway.
