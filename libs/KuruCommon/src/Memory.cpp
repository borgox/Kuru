#include "Kuru/Common/Memory.hpp"

#include <windows.h>

namespace Kuru::Common {

namespace {

[[nodiscard]] bool HasReadableProtection(DWORD protect) noexcept {
    if ((protect & PAGE_GUARD) != 0 || (protect & PAGE_NOACCESS) != 0) {
        return false;
    }

    protect &= 0xFF;
    return protect == PAGE_READONLY || protect == PAGE_READWRITE || protect == PAGE_WRITECOPY ||
           protect == PAGE_EXECUTE_READ || protect == PAGE_EXECUTE_READWRITE || protect == PAGE_EXECUTE_WRITECOPY;
}

[[nodiscard]] bool HasExecutableProtection(DWORD protect) noexcept {
    if ((protect & PAGE_GUARD) != 0 || (protect & PAGE_NOACCESS) != 0) {
        return false;
    }

    protect &= 0xFF;
    return protect == PAGE_EXECUTE || protect == PAGE_EXECUTE_READ || protect == PAGE_EXECUTE_READWRITE ||
           protect == PAGE_EXECUTE_WRITECOPY;
}

} // namespace

bool IsReadableMemory(const void* address, std::size_t size) noexcept {
    if (!address || size == 0) {
        return false;
    }

    const auto start = reinterpret_cast<std::uintptr_t>(address);
    const auto end = start + size;
    if (end < start) {
        return false;
    }

    std::uintptr_t current = start;
    while (current < end) {
        MEMORY_BASIC_INFORMATION mbi{};
        if (VirtualQuery(reinterpret_cast<LPCVOID>(current), &mbi, sizeof(mbi)) == 0) {
            return false;
        }

        if (mbi.State != MEM_COMMIT || !HasReadableProtection(mbi.Protect)) {
            return false;
        }

        const auto regionEnd = reinterpret_cast<std::uintptr_t>(mbi.BaseAddress) + mbi.RegionSize;
        if (regionEnd <= current) {
            return false;
        }
        current = regionEnd;
    }

    return true;
}

bool IsExecutableMemory(const void* address) noexcept {
    if (!address) {
        return false;
    }

    MEMORY_BASIC_INFORMATION mbi{};
    if (VirtualQuery(address, &mbi, sizeof(mbi)) == 0) {
        return false;
    }

    return mbi.State == MEM_COMMIT && HasExecutableProtection(mbi.Protect);
}

} // namespace Kuru::Common
