#pragma once

#include <Core/Containers.hpp>

namespace Kuru::Common {

template <typename T>
using TArray = Kuru::SDK::TArray<T>;

static_assert(sizeof(TArray<void*>) == 0x10);
static_assert(offsetof(TArray<void*>, Data) == 0x0);
static_assert(offsetof(TArray<void*>, Count) == 0x8);
static_assert(offsetof(TArray<void*>, Max) == 0xC);

} // namespace Kuru::Common
