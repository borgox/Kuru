#pragma once

#include "Kuru/Common/Unreal.hpp"

#include <cstdint>
#include <mutex>
#include <optional>
#include <string>

namespace Kuru::Common {

struct RocketLeagueGlobals {
    const TArray<void*>* GNames = nullptr;
    const TArray<UObjectPrefix*>* GObjects = nullptr;
    std::uintptr_t GMallocAddress = 0;
    std::uintptr_t GMallocValue = 0;
    std::uintptr_t PsyonixBuildIDAddress = 0;
    std::uintptr_t BuildDateAddress = 0;
    std::string PsyonixBuildID;
    std::string BuildDate;
};

[[nodiscard]] RocketLeagueGlobals GetRocketLeagueGlobals();
void SetRocketLeagueGlobals(const RocketLeagueGlobals& globals);
void ClearRocketLeagueGlobals();

[[nodiscard]] std::optional<RocketLeagueGlobals> TryInitializeRocketLeagueGlobalsFromGNames(
    const void* gNamesAddress,
    GlobalsValidationOptions options = {});

[[nodiscard]] std::optional<RocketLeagueGlobals> TryInitializeRocketLeagueGlobalsFromPattern(
    std::string_view gNamesPattern,
    GlobalsValidationOptions options = {});

} // namespace Kuru::Common
