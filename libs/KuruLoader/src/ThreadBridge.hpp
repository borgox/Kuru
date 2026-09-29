#pragma once

#include <functional>

// Thread bridge: lets render-thread code (GUI) and mods schedule work onto the GAME thread.

namespace Kuru::ThreadBridge {

void NotifyPresent();          // render thread: a new frame is up
void PumpFromGameThread();     // game thread (from the PE detour): runs the tick if one is pending

void EnqueueGameThread(std::function<void()> fn);           // run once, next game-thread tick
void EnqueueTimeout(std::function<void()> fn, float seconds); // run once after a delay
[[nodiscard]] bool IsGameThread();

// HostApi glue (C ABI) pushed to mods.
void HostExecute(void (*thunk)(void* user), void* user);
void HostSetTimeout(void (*thunk)(void* user), void* user, float seconds);
[[nodiscard]] bool HostIsGameThread();

} // namespace Kuru::ThreadBridge
