#pragma once

#include <Kuru/HookContext.hpp>
#include <Kuru/HostApi.hpp>
#include <Core/ObjectModel.hpp>

#include <functional>
#include <utility>

/* Mod-facing hook API. Register by function full name (e.g. "Function TAGame.Ball_TA.OnHitGoal").
 * The callback receives a HookContext&: read the caller/function, typed params (ctx.Params<T>()),
 * the result (ctx.Result<T>()), and (for PI/CF) the call frame. Pre hooks may ctx.Block() to skip
 *  the original call. The std::function stays inside the mod. only a C thunk + pointer cross to the
 * host, which keys hooks by the resolved UFunction* and dispatches on the game thread.
 * Instead of these you should use Events.
*/
namespace Kuru::Hooks {
namespace detail {

using Callable = std::function<void(HookContext&)>;

inline void Thunk(HookContext* context, void* user) {
    if (context) {
        (*static_cast<Callable*>(user))(*context);
    }
}

inline HookHandle Register(const char* functionFullName, HookTiming timing, HookDispatch dispatch,
                           Callable callable) {
    const HostApi* api = GetHostApi();
    if (!api || !api->RegisterHook) {
        return 0;
    }
    // Held for the mod's lifetime, RemoveHook cleanup of the holder is a later refinement.
    auto* holder = new Callable(std::move(callable));
    return api->RegisterHook(functionFullName, timing, dispatch, &Thunk, holder);
}

} // namespace detail

// Default: fire on whichever dispatch layer actually runs the function deduped to one call per native invocation. no need to know the layers exist.
inline HookHandle Pre(const char* functionFullName, detail::Callable callback) {
    return detail::Register(functionFullName, HookTiming::Pre, HookDispatch::Any, std::move(callback));
}
inline HookHandle Post(const char* functionFullName, detail::Callable callback) {
    return detail::Register(functionFullName, HookTiming::Post, HookDispatch::Any, std::move(callback));
}

// Advanced: pick the dispatch layer explicitly.
inline HookHandle PreVia(const char* functionFullName, HookDispatch dispatch, detail::Callable callback) {
    return detail::Register(functionFullName, HookTiming::Pre, dispatch, std::move(callback));
}
inline HookHandle PostVia(const char* functionFullName, HookDispatch dispatch, detail::Callable callback) {
    return detail::Register(functionFullName, HookTiming::Post, dispatch, std::move(callback));
}

// Advanced: ProcessInternal,  catches script->script executions PE never sees, and gives the call frame (ctx.Frame()) for the call chain.
inline HookHandle PreInternal(const char* functionFullName, detail::Callable callback) {
    return PreVia(functionFullName, HookDispatch::ProcessInternal, std::move(callback));
}
inline HookHandle PostInternal(const char* functionFullName, detail::Callable callback) {
    return PostVia(functionFullName, HookDispatch::ProcessInternal, std::move(callback));
}

inline void Remove(HookHandle handle) {
    if (const HostApi* api = GetHostApi(); api && api->RemoveHook) {
        api->RemoveHook(handle);
    }
}

/* Silence the base-class override hint for THIS mod's hooks. Call once in OnLoad (before Pre/
 * Post calls) if you intentionally hook base-class names. Only affects your mod, and resets on reload.
*/
inline void SuppressOverrideWarnings() {
    if (const HostApi* api = GetHostApi(); api && api->SuppressOverrideWarnings) {
        api->SuppressOverrideWarnings();
    }
}

} // namespace Kuru::Hooks
