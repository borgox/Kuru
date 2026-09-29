#pragma once

#include "Kuru/Common/TArray.hpp"

#include <cstddef>
#include <cstdint>
#include <optional>
#include <string>

namespace Kuru::Common {

struct FName {
    std::int32_t Index = 0;
    std::int32_t Number = 0;
};

// Minimal runtime `UObject` prefix used for global validation. Full SDK layouts live in `sdk-base`.
struct UObjectPrefix {
    void* VfTableObject = nullptr;        // 0x0000
    void* HashNext = nullptr;             // 0x0008
    std::uint64_t ObjectFlags = 0;        // 0x0010
    void* HashOuterNext = nullptr;        // 0x0018
    void* StateFrame = nullptr;           // 0x0020
    void* Linker = nullptr;               // 0x0028
    void* LinkerIndex = nullptr;          // 0x0030, RL fork stores pointer-sized data here
    std::int32_t ObjectInternalInteger{}; // 0x0038
    std::int32_t NetIndex{};              // 0x003C
    UObjectPrefix* Outer = nullptr;       // 0x0040
    FName Name{};                         // 0x0048
    UObjectPrefix* Class = nullptr;       // 0x0050
    UObjectPrefix* ObjectArchetype = nullptr; // 0x0058
};

struct GlobalsValidationOptions {
    std::int32_t minNameCount = 5'000;
    std::int32_t minObjectCount = 10'000;
    std::int32_t maxNameCount = 1'000'000;
    std::int32_t maxObjectCount = 2'000'000;
    std::int32_t objectSampleCount = 128;
    std::int32_t minimumValidObjectSamples = 16;
};

struct GlobalsValidationResult {
    bool valid = false;
    const TArray<void*>* gNames = nullptr;
    const TArray<UObjectPrefix*>* gObjects = nullptr;
    std::int32_t validObjectSamples = 0;
    std::string failureReason;
};

static_assert(sizeof(FName) == 0x8);
static_assert(sizeof(UObjectPrefix) == 0x60);
static_assert(offsetof(UObjectPrefix, Outer) == 0x40);
static_assert(offsetof(UObjectPrefix, Name) == 0x48);
static_assert(offsetof(UObjectPrefix, Class) == 0x50);

[[nodiscard]] std::optional<std::string> TryReadFNameEntryString(const TArray<void*>* gNames, FName name);
[[nodiscard]] bool LooksLikeUObject(const UObjectPrefix* object, const TArray<void*>* gNames);

// GObjects is expected at GNames + 0x48. This function validates the derived pointer before callers use it.
[[nodiscard]] GlobalsValidationResult ValidateRocketLeagueGlobalsFromGNames(
    const void* gNamesAddress,
    GlobalsValidationOptions options = {});

} // namespace Kuru::Common
