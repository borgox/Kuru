# Kuru::Events

The typed event bus. See the [events guide](../modding/guides/events.md) for the concepts.

`EventT` is either a function-call event (`PreFunctionCallEvent`, `PostFunctionCallEvent`) or any struct you
define with a `static constexpr const char* Identifier`.

## Functions

### `Register<EventT>(const char* listenerName, callback, const char* filter = "", int priority = 0) -> ListenerHandle`

Register a callable taking `EventT&`. For function-call events, `filter` is the function full name. For module
events, an empty `filter` matches every dispatch.

For module events, a nonempty listener filter must exactly match the dispatch filter. Higher priorities
run first, with registration order retained for equal priorities. Function events require a function
name. Their listener name and priority are ignored. A zero handle means registration failed.

### `Register<EventT>(const char* listenerName, ObjT* obj, void (ObjT::*method)(EventT&), const char* filter = "", int priority = 0) -> ListenerHandle`

The same, binding a member function.

### `Dispatch<EventT>(EventT& event, const char* filter = "")`

Fan a module-defined event out to its listeners. Function-call events are produced by the engine, not
dispatched by hand.

Module dispatch is synchronous on the thread calling it. The payload must remain valid until dispatch
returns. Listeners must not keep references to it afterward. Nested dispatch is allowed. Shared event
identifiers require the same payload definition in all participating mods.

### `Unregister<EventT>(ListenerHandle handle)`

Stop a listener. Templated on the event type so it routes to the right registry.

The current SDK requires host API v4 for registration. Listener captures are released after removal
and after active dispatch copies finish. Host-owned listeners are also removed on mod teardown.

## Built-in events

- `PreFunctionCallEvent`: produced before a function runs. Has `ctx`, the same `HookContext` a raw hook gets.
- `PostFunctionCallEvent`: produced after a function runs. Also has `ctx`.

`Events::ListenerHandle` aliases `HookHandle` (64-bit; zero is invalid). `Kuru::IEvent` is an optional
marker base with a virtual destructor; module payloads do not have to inherit it.
