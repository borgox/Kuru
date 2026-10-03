# Delegates

A delegate is a function bound at runtime - a "call this later" handle. UnrealScript uses them for callbacks
and events: "when login completes, call this." Rocket League's delegate system is one of the more forked parts
of the engine, and what's here comes from reverse-engineering it directly.

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
pointer to a `TArray<FScriptDelegate>` - the multicast invocation list. The engine allocates that array
only when more than one handler is bound, so with zero or one handler the pointer is null and the inline
binding is used. This is the type Kuru calls `FMultiScriptDelegate`, and it's why delegate fields on a
class are spaced `0x18` apart.

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
