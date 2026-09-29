#pragma once

#include <cstdint>
#include <span>

namespace Kuru::Common {

// UE3 x64 TArray layout. Keep this natural-layout type tiny and verified.
template <typename T>
struct TArray {
    T* Data = nullptr;
    std::int32_t Count = 0;
    std::int32_t Max = 0;

    [[nodiscard]] T* data() noexcept { return Data; }
    [[nodiscard]] const T* data() const noexcept { return Data; }
    [[nodiscard]] std::int32_t size() const noexcept { return Count; }
    [[nodiscard]] bool empty() const noexcept { return Count <= 0; }

    [[nodiscard]] T& operator[](std::int32_t index) noexcept { return Data[index]; }
    [[nodiscard]] const T& operator[](std::int32_t index) const noexcept { return Data[index]; }

    [[nodiscard]] std::span<T> span() noexcept {
        return (Data && Count > 0) ? std::span<T>(Data, static_cast<std::size_t>(Count)) : std::span<T>();
    }

    [[nodiscard]] std::span<const T> span() const noexcept {
        return (Data && Count > 0) ? std::span<const T>(Data, static_cast<std::size_t>(Count)) : std::span<const T>();
    }
};

static_assert(sizeof(TArray<void*>) == 0x10);
static_assert(offsetof(TArray<void*>, Data) == 0x0);
static_assert(offsetof(TArray<void*>, Count) == 0x8);
static_assert(offsetof(TArray<void*>, Max) == 0xC);

} // namespace Kuru::Common
