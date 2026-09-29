#include "ThreadBridge.hpp"

#include "ModRegistry.hpp"

#include <Kuru/Mod.hpp>

#include <windows.h>

#include <atomic>
#include <chrono>
#include <mutex>
#include <vector>

namespace Kuru::ThreadBridge {
namespace {

std::atomic<bool> gTickPending{false};
std::atomic<unsigned long> gGameThreadId{0};

std::mutex gMutex;
std::vector<std::function<void()>> gTasks;

struct Timeout {
    std::function<void()> fn;
    double due;
};
std::vector<Timeout> gTimeouts;

double NowSeconds() {
    using namespace std::chrono;
    return duration<double>(steady_clock::now().time_since_epoch()).count();
}

double gLastTick = 0.0;

} // namespace

void NotifyPresent() {
    gTickPending.store(true, std::memory_order_relaxed);
}

void PumpFromGameThread() {
    // Cheap on the hot path: most PE calls between frames just do this atomic and return.
    if (!gTickPending.exchange(false, std::memory_order_relaxed)) {
        return;
    }
    gGameThreadId.store(GetCurrentThreadId(), std::memory_order_relaxed);

    const double now = NowSeconds();
    const float dt = gLastTick > 0.0 ? static_cast<float>(now - gLastTick) : 0.0f;
    gLastTick = now;

    // Immediate tasks (Execute).
    std::vector<std::function<void()>> tasks;
    {
        std::scoped_lock lock(gMutex);
        tasks.swap(gTasks);
    }
    for (auto& task : tasks) {
        try { task(); } catch (...) {}
    }

    // Due timeouts.
    std::vector<std::function<void()>> due;
    {
        std::scoped_lock lock(gMutex);
        for (auto it = gTimeouts.begin(); it != gTimeouts.end();) {
            if (now >= it->due) {
                due.push_back(std::move(it->fn));
                it = gTimeouts.erase(it);
            } else {
                ++it;
            }
        }
    }
    for (auto& task : due) {
        try { task(); } catch (...) {}
    }

    // Mod OnUpdate (game thread, once per frame).
    for (const Kuru::ModRegistry::Entry& mod : Kuru::ModRegistry::Entries()) {
        if (mod.instance && mod.enabled) {
            try { mod.instance->OnUpdate(dt); } catch (...) {}
        }
    }
}

void EnqueueGameThread(std::function<void()> fn) {
    std::scoped_lock lock(gMutex);
    gTasks.push_back(std::move(fn));
}

void EnqueueTimeout(std::function<void()> fn, float seconds) {
    std::scoped_lock lock(gMutex);
    gTimeouts.push_back({std::move(fn), NowSeconds() + seconds});
}

bool IsGameThread() {
    return GetCurrentThreadId() == gGameThreadId.load(std::memory_order_relaxed);
}

void HostExecute(void (*thunk)(void*), void* user) {
    if (thunk) {
        EnqueueGameThread([thunk, user] { thunk(user); });
    }
}

void HostSetTimeout(void (*thunk)(void*), void* user, float seconds) {
    if (thunk) {
        EnqueueTimeout([thunk, user] { thunk(user); }, seconds);
    }
}

bool HostIsGameThread() {
    return IsGameThread();
}

} // namespace Kuru::ThreadBridge
