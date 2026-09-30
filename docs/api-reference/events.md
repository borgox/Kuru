# Kuru::Events

The typed event bus. See the [events guide](../modding/guides/events.md) for the concepts.

`EventT` is either a function-call event (`PreFunctionCallEvent`, `PostFunctionCallEvent`) or any struct you
define with a `static constexpr const char* Identifier`.

## Functions

`Register<EventT>(const char* listenerName, callback, const char* filter = "", int priority = 0) -> ListenerHandle`
Register a callable taking `EventT&`. For function-call events, `filter` is the function full name. For module
events, an empty `filter` matches every dispatch.

`Register<EventT>(const char* listenerName, ObjT* obj, void (ObjT::*method)(EventT&), const char* filter = "", int priority = 0) -> ListenerHandle`
The same, binding a member function.

`Dispatch<EventT>(EventT& event, const char* filter = "")`
Fan a module-defined event out to its listeners. Function-call events are produced by the engine, not
dispatched by hand.

`Unregister<EventT>(ListenerHandle handle)`
Stop a listener. Templated on the event type so it routes to the right registry.

## Built-in events

`PreFunctionCallEvent`  produced before a function runs. Has `ctx`, the same `HookContext` a raw hook gets.
`PostFunctionCallEvent`  produced after a function runs. Also has `ctx`.
