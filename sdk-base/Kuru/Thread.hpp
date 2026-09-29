#pragma once

#include <Kuru/HostApi.hpp>

#include <functional>
#include <utility>

/* Thread bridge for mods. Game objects/functions must only be touched on the game thread. GUI runs
 * on the render thread. Use Execute() to bounce work from the render thread (e.g. an OnRender button)
 * onto the game thread. Callbacks run once, on the next game-thread frame.
*/
namespace Kuru {
namespace detail {

inline void RunOnceThunk(void* user) {
    auto* fn = static_cast<std::function<void()>*>(user);
    (*fn)();
    delete fn;  // one-shot
}

} // namespace detail

inline void Execute(std::function<void()> fn) {
    const HostApi* api = GetHostApi();
    if (!api || !api->Execute) {
        return;
    }
    api->Execute(&detail::RunOnceThunk, new std::function<void()>(std::move(fn)));
}

inline void SetTimeout(std::function<void()> fn, float seconds) {
    const HostApi* api = GetHostApi();
    if (!api || !api->SetTimeout) {
        return;
    }
    api->SetTimeout(&detail::RunOnceThunk, new std::function<void()>(std::move(fn)), seconds);
}

[[nodiscard]] inline bool IsGameThread() {
    const HostApi* api = GetHostApi();
    return api && api->IsGameThread ? api->IsGameThread() : false;
}

} // namespace Kuru
