#pragma once

#include <Kuru/HookContext.hpp>
#include <Kuru/HostApi.hpp>

#include <cstdint>
#include <type_traits>
#include <utility>

/* Typed event bus Two kinds of event:
 *
 *   1. Function-call events, PreFunctionCallEvent / PostFunctionCallEvent. Every VM-layer hook is a
 *      producer of these. A listener filters by a function's full name and receives the live
 *      HookContext. These route through the (validated) hook engine, so canonical-layer routing and
 *      recursion dedup are preserved for free.
 *
 *   2. Module-defined events, any struct with a `static constexpr const char* Identifier`. A module
 *      dispatches one with Events::Dispatch<T>(ev) and other modules listen with Events::Register<T>.
 *      These fan out through the host's general listener registry.
 *
 * Usage:
 *   Events::Register<PostFunctionCallEvent>("on_goal", this, &Mod::OnGoal, "Function TAGame.Ball_TA.OnHitGoal");
 *   Events::Register<MyEvent>("on_my_event", this, &Mod::OnMyEvent);      // module event, no filter
 *   MyEvent ev{...}; Events::Dispatch<MyEvent>(ev);                        // fan out to listeners
 *   Events::Unregister<MyEvent>(handle);                                   // stop listening
 *
 * Kuru::Hooks::Pre/Post stays as the terse wrapper for the common function-hook case.
 */
namespace Kuru {

// Marker base: module events may inherit it for documentation, it is not required.
struct IEvent {
    virtual ~IEvent() = default;
};

// The two function-call events.
struct PreFunctionCallEvent {
    static constexpr const char* Identifier = "kuru:function_call_pre";
    HookContext& ctx;
};
struct PostFunctionCallEvent {
    static constexpr const char* Identifier = "kuru:function_call_post";
    HookContext& ctx;
};

namespace Events {

using ListenerHandle = HookHandle;

namespace detail {

template <typename EventT, typename Fn>
struct Holder {
    Fn fn;
};

// Module-event thunk: the host hands us the event payload (an EventT*), call the mod's callable.
template <typename EventT, typename Fn>
void ModuleThunk(void* payload, void* user) {
    auto* holder = static_cast<Holder<EventT, Fn>*>(user);
    holder->fn(*static_cast<EventT*>(payload));
}

// Function-event thunk: matches HookCallbackFn (HookContext*, user); wrap the context into the event.
template <typename EventT, typename Fn>
void FunctionThunk(HookContext* context, void* user) {
    auto* holder = static_cast<Holder<EventT, Fn>*>(user);
    EventT event{*context};
    holder->fn(event);
}

template <typename EventT>
struct is_function_event : std::false_type {};
template <>
struct is_function_event<PreFunctionCallEvent> : std::true_type {};
template <>
struct is_function_event<PostFunctionCallEvent> : std::true_type {};

}  // namespace detail

/**
 * Register a callable (lambda or free function) taking `EventT&`.
 *    - listenerName: id for this listener (module events); ignored for function events.
 *    - filter: for function events, the function full name (required); for module events, "" = all.
 *    - priority: higher fires first (module events).
 */
template <typename EventT, typename Fn>
ListenerHandle Register(const char* listenerName, Fn fn, const char* filter = "", int priority = 0) {
    const HostApi* api = GetHostApi();
    if (!api || api->Version < 4) {
        return 0;
    }
    using Callable = std::decay_t<Fn>;
    using Holder = detail::Holder<EventT, Callable>;
    auto* holder = new Holder{std::move(fn)};
    const auto cleanup = +[](void* user) { delete static_cast<Holder*>(user); };
    if constexpr (detail::is_function_event<EventT>::value) {
        const HookTiming timing =
            std::is_same_v<EventT, PreFunctionCallEvent> ? HookTiming::Pre : HookTiming::Post;
        (void)listenerName;
        (void)priority;
        if (!api->RegisterOwnedHook) { delete holder; return 0; }
        return api->RegisterOwnedHook(filter, timing, HookDispatch::Any, &detail::FunctionThunk<EventT, Callable>, holder, cleanup);
    } else {
        if (!api->RegisterOwnedListener) {
            delete holder;
            return 0;
        }
        return api->RegisterOwnedListener(EventT::Identifier, listenerName, &detail::ModuleThunk<EventT, Callable>,
                                          holder, filter, priority, cleanup);
    }
}

// Member-function overload: Register<EventT>("name", this, &Class::Method, filter, priority).
template <typename EventT, typename ObjT>
ListenerHandle Register(const char* listenerName, ObjT* obj, void (ObjT::*method)(EventT&),
                        const char* filter = "", int priority = 0) {
    return Register<EventT>(
        listenerName, [obj, method](EventT& e) { (obj->*method)(e); }, filter, priority);
}

// Stop a listener. Templated on the event type so it routes to the right registry.
template <typename EventT>
void Unregister(ListenerHandle handle) {
    const HostApi* api = GetHostApi();
    if (!api || handle == 0) {
        return;
    }
    if constexpr (detail::is_function_event<EventT>::value) {
        if (api->RemoveHook) {
            api->RemoveHook(handle);
        }
    } else {
        if (api->UnregisterListener) {
            api->UnregisterListener(handle);
        }
    }
}

// Dispatch a module-defined event to every matching listener.
template <typename EventT>
void Dispatch(EventT& event, const char* filter = "") {
    const HostApi* api = GetHostApi();
    if (api && api->DispatchEvent) {
        api->DispatchEvent(EventT::Identifier, filter, &event);
    }
}

}  // namespace Events
}  // namespace Kuru
