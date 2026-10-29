# Delegates

A delegate is a function bound at runtime - a "call this later" handle. UnrealScript uses them for callbacks
and events: "when login completes, call this." Rocket League's delegate system is one of the more forked parts
of the engine. This page distinguishes the SDK's current representation from engine behavior that
still needs a retained binary trace or other verification.

## FScriptDelegate

The SDK models a single binding as `FScriptDelegate`: an object pointer and a function name,
occupying 16 bytes:

```text
0x00  Object          UObject* to call the function on
0x08  FunctionName    FName of the function
```

This describes the SDK type, it does not establish the internal format of every delegate property
in the current game.

## Delegate property storage

`FMultiScriptDelegate` is the SDK's legacy 24-byte model: a single binding followed by a pointer
to a multicast list. Its `Broadcast` helper follows that modeled representation. Do not cast a
larger game property to this type or call the helper on unverified engine storage.

Read-only reflection on 2026-10-29 reports **32-byte** delegate properties in the engine and
viewport classes. The handwritten SDK uses opaque `FEngineDelegateStorage` for those fields and
updates subsequent offsets accordingly. Their invocation format remains unverified.

The generator compares each delegate property's reflected element size with the SDK type's size.
Matching 24-byte slots keep the typed model; other sizes are emitted as opaque bytes, including
in fixed arrays and function parameter blocks. This preserves surrounding field offsets without
assuming that a new footprint has the old invocation semantics.

## Delegate entry points

The engine has delegate entry points in addition to the three layers currently intercepted by Kuru:

- **`ProcessDelegate`** handles a delegate call, and for a multicast delegate loops the invocation list.
- **`CallDelegate`** invokes one bound target - it resolves the binding to a `UFunction` and dispatches it.

The current hook engine does not install separate detours on either delegate entry point. A hook on
the bound function fires only if the engine reaches that function through an intercepted layer
eligible for that hook's dispatch selection.

The SDK's `FScriptDelegate::ProcessDelegate` helper requires a ProcessDelegate runtime binding. The
current loader bootstrap supplies ProcessEvent but does not populate that delegate binding, so the
helper silently does nothing with the bindings supplied by the current loader.

## NetDelegate

Earlier research associated `0x20000000` with network-related delegate handlers. The current
`Core/Flags.hpp` still names the bit `FUNC_EditorOnly` and the hook engine does not implement a
NetDelegate routing branch. Correlation with RPC/message handler names does not establish that a
flagged call crosses the network. Treat the interpretation, routing rules, and connection to the
extra delegate storage as unverified until supported by a retained trace or disassembly.

Sources: [Delegates.hpp](../../../sdk-base/Core/Delegates.hpp),
[Flags.hpp](../../../sdk-base/Core/Flags.hpp), and
[ObjectModel.cpp](../../../sdk-base/src/ObjectModel.cpp). See
[SDK verification annotations](../../../sdk-base/Core/Script.hpp).
