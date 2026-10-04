#pragma once

#include <atomic>
#include <windows.h>

namespace Kuru::Lifecycle {

inline std::atomic<bool> stopping{false};

// Keep trampolines and DLL code alive until calls that entered a detour return.
class Activity {
public:
    class Scope {
    public:
        explicit Scope(Activity& owner) : owner_(owner) { owner_.calls_.fetch_add(1); }
        ~Scope() { owner_.calls_.fetch_sub(1); }
        Scope(const Scope&) = delete;
        Scope& operator=(const Scope&) = delete;
    private:
        Activity& owner_;
    };

    bool WaitForIdle(DWORD timeoutMs = 5000) const {
        const ULONGLONG deadline = GetTickCount64() + timeoutMs;
        while (calls_.load() != 0) {
            if (GetTickCount64() >= deadline) return false;
            Sleep(1);
        }
        return true;
    }
private:
    std::atomic<unsigned> calls_{0};
};

} // namespace Kuru::Lifecycle
