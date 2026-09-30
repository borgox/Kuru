# Delegates

A delegate is a function bound at runtime - a "call this later" handle. UnrealScript uses them for callbacks
and events: "when login completes, call this." Rocket League's delegate system is one of the more forked parts
of the engine, and what's here comes from reverse-engineering it directly.

## FScriptDelegate

A single-cast delegate is an `FScriptDelegate`. In stock UE3 it's two fields - the object to call and the name
of the function on it - 16 bytes:

```
0x00  Object          UObject* to call the function on
0x08  FunctionName    FName of the function
```

In the Rocket League fork it's **24 bytes**: there's an extra 8-byte field at offset `0x10`.

## The field at 0x10

That extra field is a **native binding**. When Psyonix binds a delegate to C++ code rather than to an
UnrealScript function, they store the C++ callback there - and clear `Object` and `FunctionName`, because the
two representations are mutually exclusive. A delegate is bound *either* the script way (object + function
name) *or* the native way (the pointer at `0x10`).

In practice most of the engine's event delegates (`__Event*__Delegate`) are wired to native handlers, so on a
live game the vast majority have `Object`/`FunctionName` null and the `0x10` pointer set. That's exactly what
the Objects tab shows if you inspect a delegate property, and it's how the field's meaning was recovered.

## Multicast delegates

A multicast delegate (`FMulticastScriptDelegate`) is a `TArray` of `FScriptDelegate` - an invocation list.
Calling it invokes every entry. It's the "many listeners" form; a single-cast delegate is the "one listener"
form.

## The two dispatch layers

Delegates get their own entries in the [dispatch layer](05-function-dispatch-layers.md) table:

- **`ProcessDelegate`** handles a delegate call, and for a multicast delegate loops the invocation list.
- **`CallDelegate`** invokes one bound target - it resolves the binding to a `UFunction` and dispatches it.

For an ordinary delegate this ends in a normal local call. Kuru hooks these layers like any other, resolving
the bound function so a name-based hook on it fires when the delegate runs.

## NetDelegate

One `UFunction` flag, `0x20000000` - a bit that means "editor-only" in stock UE3 but is repurposed in the fork
— marks a **networked** delegate. The flagged functions turn out to be RPC and network-message handlers
(`Handle*RPC`, `Handle*Message`, login/party/matchmaking callbacks). A plain delegate is invoked locally; a net
delegate's call is instead routed through the replication path so it crosses the network. That routing is the
extra branch in the delegate dispatch, and it's why the fork carries the extra per-delegate state to begin with.
