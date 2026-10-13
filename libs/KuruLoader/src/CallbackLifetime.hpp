#pragma once
#include <atomic>

namespace Kuru {
// Shared by a registry entry and any dispatch snapshots. Destruction runs in the mod.
struct CallbackLifetime {
    void* user;
    void (*cleanup)(void*);
    std::atomic<bool> removed{false};
    CallbackLifetime(void* user, void (*cleanup)(void*)) : user(user), cleanup(cleanup) {}
    ~CallbackLifetime() { if (cleanup) cleanup(user); }
};
}
