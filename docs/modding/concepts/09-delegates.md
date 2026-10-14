# Delegates

A delegate is a function bound at runtime - a "call this later" handle. UnrealScript uses them for callbacks
and events: "when login completes, call this." Rocket League's delegate system is one of the more forked parts
of the engine. This page distinguishes the SDK's current representation from engine behavior that
still needs a retained binary trace or other verification.

## FScriptDelegate

A single-cast delegate is an `FScriptDelegate`: the object to call and the name of the function on it, 16
bytes, exactly as in stock UE3:

```
0x00  Object          UObject* to call the function on
0x08  FunctionName    FName of the function
```

## Delegate properties: FMultiScriptDelegate

A delegate *property* on a Rocket League object isn't a bare `FScriptDelegate` - it's **24 bytes**. The
first 16 are an inline single binding (object + function name); the extra 8-byte field at `0x10` is a
pointer to a `TArray<FScriptDelegate>` in the current SDK model. This is `FMultiScriptDelegate`, size
`0x18`. Its helper `Broadcast` walks a nonempty multicast list when present and otherwise invokes the
inline binding. That helper does not establish exactly when the game's own allocator creates or
switches representations; don't infer that rule from a null pointer or the C++ size assertion alone.

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
