#pragma once

#include <MinHook.h>

// Thin RAII wrapper over MinHook so install sites don't repeat the create/enable/disable/remove dance and
// so the library init/teardown is reference counted across the hook subsystems (engine + GUI overlay).

namespace Kuru {

// MinHook's process-wide init/teardown, reference counted by the number of live THook objects. THook calls
// these itself, so the library is initialized on the first installed hook and uninitialized when the last
// one is removed, regardless of which subsystem owns them.
namespace MinHook {

inline int& RefCount() noexcept {
    static int count = 0;  // one shared instance: RefCount() is an inline function with external linkage
    return count;
}
inline void Acquire() {
    if (RefCount()++ == 0) {
        MH_Initialize();
    }
}
inline void Release() {
    if (RefCount() > 0 && --RefCount() == 0) {
        MH_Uninitialize();
    }
}

} // namespace MinHook

// One MinHook detour. Install() creates and enables it; Remove() (and the destructor) disables and removes
// it. Each installed hook holds one MinHook reference, so callers never init/uninit the library themselves.
// Non-copyable; a given target is owned by exactly one THook.
class THook {
public:
    THook() = default;
    ~THook() { Remove(); }
    THook(const THook&) = delete;
    THook& operator=(const THook&) = delete;

    // `original` receives the trampoline used to call the un-hooked function. Returns false and installs
    // nothing on a null target or any MinHook failure.
    [[nodiscard]] bool Install(void* target, void* detour, void** original) {
        Remove();
        if (!target) {
            return false;
        }
        MinHook::Acquire();  // ensure the library is initialized before creating the hook
        if (MH_CreateHook(target, detour, original) != MH_OK || MH_EnableHook(target) != MH_OK) {
            MH_RemoveHook(target);  // never leave a half-installed hook behind
            MinHook::Release();
            return false;
        }
        target_ = target;
        return true;
    }

    void Remove() {
        if (target_) {
            MH_DisableHook(target_);
            MH_RemoveHook(target_);
            target_ = nullptr;
            MinHook::Release();
        }
    }

    [[nodiscard]] bool Installed() const noexcept { return target_ != nullptr; }

private:
    void* target_ = nullptr;
};

} // namespace Kuru
