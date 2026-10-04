#pragma once

#include <MinHook.h>

namespace Kuru {


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

// One MinHook detour. Install() creates and enables it; Remove() (and the destructor) disables and removes it.
class THook {
public:
    THook() = default;
    ~THook() { Remove(); }
    THook(const THook&) = delete;
    THook& operator=(const THook&) = delete;

    // `original` receives the trampoline used to call the un-hooked function. Returns false and installs nothing on a null target or any MinHook failure.
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

    // Disable entry without freeing the trampoline used by in-flight calls.
    [[nodiscard]] bool Disable() {
        if (!target_) return true;
        const MH_STATUS status = MH_DisableHook(target_);
        return status == MH_OK || status == MH_ERROR_DISABLED;
    }

    [[nodiscard]] bool Installed() const noexcept { return target_ != nullptr; }

private:
    void* target_ = nullptr;
};

} // namespace Kuru
