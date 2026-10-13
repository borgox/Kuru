#pragma once

#include <Kuru/HostApi.hpp>

#include <functional>
#include <utility>

/* Queue work on the game thread. Pending tasks belong to this mod instance and are canceled
 * on disable/reload. Capture only objects that remain alive until execution or cancellation.
 * Host API v3 owns the holder until it calls the mod-provided cleanup function.
 */
namespace Kuru {
namespace detail {

inline void RunOnceThunk(void* user) {
    auto* fn = static_cast<std::function<void()>*>(user);
    (*fn)();

}

inline void DeleteTask(void* user) { delete static_cast<std::function<void()>*>(user); }

} // namespace detail

inline void SetTimeout(std::function<void()> fn, float seconds) {
    const HostApi* api = GetHostApi();
    if (!api || api->Version < 3 || !api->QueueTask) return;
    api->QueueTask(GetTaskOwner(), &detail::RunOnceThunk,
                   new std::function<void()>(std::move(fn)), &detail::DeleteTask, seconds);
}

inline void Execute(std::function<void()> fn) {
    SetTimeout(std::move(fn), 0.0f);
}

[[nodiscard]] inline bool IsGameThread() {
    const HostApi* api = GetHostApi();
    return api && api->IsGameThread ? api->IsGameThread() : false;
}

} // namespace Kuru
