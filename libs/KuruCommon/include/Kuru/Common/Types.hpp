#pragma once

#include <cstddef>
#include <cstdint>

namespace Kuru::Common {

using u8 = std::uint8_t;
using u16 = std::uint16_t;
using u32 = std::uint32_t;
using u64 = std::uint64_t;
using i8 = std::int8_t;
using i16 = std::int16_t;
using i32 = std::int32_t;
using i64 = std::int64_t;

struct AddressRange {
    std::uintptr_t begin = 0;
    std::uintptr_t end = 0;

    [[nodiscard]] constexpr bool contains(std::uintptr_t address, std::size_t size = 1) const noexcept {
        return begin <= address && address <= end && size <= (end - address);
    }
};

} // namespace Kuru::Common
