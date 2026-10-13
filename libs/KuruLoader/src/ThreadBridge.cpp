#include "ThreadBridge.hpp"

#include "ModRegistry.hpp"
#include "Lifecycle.hpp"

#include <Kuru/Mod.hpp>

#include <windows.h>

#include <atomic>
#include <chrono>
#include <mutex>
#include <vector>
#include <memory>
#include <unordered_map>
#include <unordered_set>
#include <algorithm>

namespace Kuru::ThreadBridge {
namespace {

std::atomic<bool> gTickPending{false};
std::atomic<unsigned long> gGameThreadId{0};

std::mutex gMutex;
struct Task {
    void* owner;
    std::function<void()> fn;
};
using TaskPtr = std::shared_ptr<Task>;
std::vector<TaskPtr> gTasks;
std::unordered_map<void*, std::vector<std::weak_ptr<Task>>> gOwned;
std::unordered_set<void*> gCanceled;
std::function<void()> gShutdownTask;

struct Timeout {
    TaskPtr task;
    double due;
};
std::vector<Timeout> gTimeouts;

double NowSeconds() {
    using namespace std::chrono;
    return duration<double>(steady_clock::now().time_since_epoch()).count();
}

double gLastTick = 0.0;

void RunTask(const TaskPtr& task) {
    std::function<void()> fn;
    {
        std::scoped_lock lock(gMutex);
        fn = std::move(task->fn);
    }
    if (fn) {
        Kuru::ModRegistry::ScopedCurrentMod owner(static_cast<Kuru::IMod*>(task->owner));
        try { fn(); } catch (...) {}
    }
}

} // namespace

void NotifyPresent() {
    gTickPending.store(true, std::memory_order_relaxed);
}

void PumpFromGameThread() {
    if (Kuru::Lifecycle::stopping.load()) {
        const auto threadId = gGameThreadId.load();
        if (threadId == 0 || threadId != GetCurrentThreadId()) return;
        std::function<void()> teardown;
        {
            std::scoped_lock lock(gMutex);
            teardown = std::move(gShutdownTask);
            gShutdownTask = {};
        }
        if (teardown) teardown();
        return;
    }
    // Cheap on the hot path: most PE calls between frames just do this atomic and return.
    if (!gTickPending.exchange(false, std::memory_order_relaxed)) {
        return;
    }
    gGameThreadId.store(GetCurrentThreadId(), std::memory_order_relaxed);

    const double now = NowSeconds();
    const float dt = gLastTick > 0.0 ? static_cast<float>(now - gLastTick) : 0.0f;
    gLastTick = now;

    // Immediate tasks (Execute).
    std::vector<TaskPtr> tasks;
    {
        std::scoped_lock lock(gMutex);
        tasks.swap(gTasks);
    }
    for (auto& task : tasks) {
        if (Kuru::Lifecycle::stopping.load()) break;
        RunTask(task);
    }

    // Due timeouts.
    std::vector<TaskPtr> due;
    {
        std::scoped_lock lock(gMutex);
        for (auto it = gTimeouts.begin(); it != gTimeouts.end();) {
            if (now >= it->due) {
                due.push_back(std::move(it->task));
                it = gTimeouts.erase(it);
            } else {
                ++it;
            }
        }
    }
    for (auto& task : due) {
        if (Kuru::Lifecycle::stopping.load()) break;
        RunTask(task);
    }

    // Mod OnUpdate (game thread, once per frame).
    for (const Kuru::ModRegistry::Entry& mod : Kuru::ModRegistry::Entries()) {
        if (Kuru::Lifecycle::stopping.load()) break;
        if (mod.instance && mod.enabled) {
            Kuru::ModRegistry::ScopedCurrentMod owner(mod.instance);
            try { mod.instance->OnUpdate(dt); } catch (...) {}
        }
    }
}

void Queue(std::function<void()> fn, void* owner, float seconds) {
    auto task = std::make_shared<Task>(Task{owner, std::move(fn)});
    std::scoped_lock lock(gMutex);
    if (Kuru::Lifecycle::stopping.load() || (owner && gCanceled.contains(owner))) return;
    if (owner) {
        auto& owned = gOwned[owner];
        std::erase_if(owned, [](const auto& weak) { return weak.expired(); });
        owned.push_back(task);
    }
    if (seconds <= 0.0f) gTasks.push_back(task);
    else gTimeouts.push_back({task, NowSeconds() + seconds});
}

void EnqueueGameThread(std::function<void()> fn) { Queue(std::move(fn), nullptr, 0); }
void EnqueueTimeout(std::function<void()> fn, float seconds) { Queue(std::move(fn), nullptr, seconds); }

void BeginForMod(void* owner) {
    std::scoped_lock lock(gMutex);
    gCanceled.erase(owner);
}

void CancelForMod(void* owner) {
    if (!owner) return;
    std::vector<std::function<void()>> release;
    {
        std::scoped_lock lock(gMutex);
        gCanceled.insert(owner);
        if (auto it = gOwned.find(owner); it != gOwned.end()) {
            for (auto& weak : it->second)
                if (auto task = weak.lock()) release.push_back(std::move(task->fn));
            gOwned.erase(it);
        }
        std::erase_if(gTasks, [owner](const auto& task) { return task->owner == owner; });
        std::erase_if(gTimeouts, [owner](const auto& timeout) { return timeout.task->owner == owner; });
    }
    // Mod destructors may reenter the host. Release outside the queue lock, before DLL unload.
}

void HostQueueTask(void* owner, void (*thunk)(void*), void* user,
                   void (*cleanup)(void*), float seconds) {
    auto storage = std::shared_ptr<void>(user, [cleanup](void* value) { if (cleanup) cleanup(value); });
    if (thunk) Queue([thunk, storage] { thunk(storage.get()); }, owner, seconds);
}

bool IsGameThread() {
    return GetCurrentThreadId() == gGameThreadId.load(std::memory_order_relaxed);
}

void HostExecute(void (*thunk)(void*), void* user) {
    if (thunk) {
        Queue([thunk, user] { thunk(user); }, Kuru::ModRegistry::CurrentMod(), 0);
    }
}

void HostSetTimeout(void (*thunk)(void*), void* user, float seconds) {
    if (thunk) {
        Queue([thunk, user] { thunk(user); }, Kuru::ModRegistry::CurrentMod(), seconds);
    }
}

bool HostIsGameThread() {
    return IsGameThread();
}

void RequestShutdown(std::function<void()> teardown) {
    std::vector<std::function<void()>> release;
    std::vector<TaskPtr> tasks;
    std::vector<Timeout> timeouts;
    {
        std::scoped_lock lock(gMutex);
        for (auto& [owner, owned] : gOwned) {
            gCanceled.insert(owner);
            for (auto& weak : owned)
                if (auto task = weak.lock()) release.push_back(std::move(task->fn));
        }
        gOwned.clear();
        tasks.swap(gTasks);
        timeouts.swap(gTimeouts);
        gShutdownTask = std::move(teardown);
    }
}

} // namespace Kuru::ThreadBridge
