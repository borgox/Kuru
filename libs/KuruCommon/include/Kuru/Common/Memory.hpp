#pragma once

#include "Kuru/Common/TArray.hpp"
#include "Kuru/Common/Types.hpp"

#include <cstddef>
#include <cstdint>
#include <span>

namespace Kuru::Common {

struct TArrayValidationOptions {
    std::int32_t minCount = 0;
    std::int32_t maxCount = 10'000'000;
    bool allowNullDataWhenEmpty = true;
};

[[nodiscard]] bool IsReadableMemory(const void* address, std::size_t size) noexcept;
[[nodiscard]] bool IsExecutableMemory(const void* address) noexcept;

// Structural validation only: pointer readable, Count/Max sane, Data readable for Count items.
template <typename T>
[[nodiscard]] bool LooksLikeTArray(const TArray<T>* array, TArrayValidationOptions options = {}) noexcept {
    if (!IsReadableMemory(array, sizeof(TArray<T>))) {
        return false;
    }

    const auto count = array->Count;
    const auto max = array->Max;

    if (count < options.minCount || count > options.maxCount) {
        return false;
    }

    if (max < count || max > options.maxCount) {
        return false;
    }

    if (!array->Data) {
        return options.allowNullDataWhenEmpty && count == 0;
    }

    if (count == 0) {
        return true;
    }

    return IsReadableMemory(array->Data, sizeof(T) * static_cast<std::size_t>(count));
}

} // namespace Kuru::Common
