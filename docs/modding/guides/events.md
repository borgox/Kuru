# Events

The event bus is a typed layer over hooks and mod-to-mod messaging. There are two kinds of event.

## Function-call events

`PreFunctionCallEvent` and `PostFunctionCallEvent` are the typed form of a hook. Register with a function
name as the filter:

```cpp
Kuru::Events::Register<Kuru::PostFunctionCallEvent>(
    "on_goal", [](Kuru::PostFunctionCallEvent& e) {
        // e.ctx is the same HookContext a raw hook gets
    },
    "Function TAGame.Ball_TA.OnHitGoal");
```

This does the same thing as `Hooks::Post(...)` - the hook API is just the shorter spelling. You never
dispatch these yourself, the game produces them.

## Module events

Your own events are any struct with a `static constexpr const char* Identifier`. One mod dispatches them,
any mod can listen:

```cpp
struct RoundStarted {
    static constexpr const char* Identifier = "mymod:round_started";
    int round;
};

// producer
RoundStarted e{3};
Kuru::Events::Dispatch<RoundStarted>(e);

// listener, in this mod or another
Kuru::Events::Register<RoundStarted>("on_round", [](RoundStarted& e) {
    Kuru::Info("round " + std::to_string(e.round));
});
```

Put the event struct in a shared header if another mod needs to listen. Stop listening with
`Kuru::Events::Unregister<RoundStarted>(handle)`.

For when to reach for events over plain hooks, and how the bus works underneath, see
[the hook engine](../concepts/07-the-hook-engine.md).
