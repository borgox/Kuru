#pragma once

#include <functional>

// Thread bridge: schedules host and mod work onto the game thread.

namespace Kuru::ThreadBridge {

void NotifyPresent();          // render thread: a new frame is up
void PumpFromGameThread();     // game thread (from the PE detour): runs the tick if one is pending

void EnqueueGameThread(std::function<void()> fn);           // run once, next game-thread tick
void EnqueueTimeout(std::function<void()> fn, float seconds); // run once after a delay
[[nodiscard]] bool IsGameThread();
// Drops pending user work, then runs host teardown on the established dispatcher thread.
void RequestShutdown(std::function<void()> teardown);
void CancelForMod(void* owner);
void BeginForMod(void* owner);
void HostQueueTask(void* owner, void (*thunk)(void*), void* user,
                   void (*cleanup)(void*), float seconds);

// HostApi glue (C ABI) pushed to mods.
void HostExecute(void (*thunk)(void* user), void* user);
void HostSetTimeout(void (*thunk)(void* user), void* user, float seconds);
[[nodiscard]] bool HostIsGameThread();

} // namespace Kuru::ThreadBridge
