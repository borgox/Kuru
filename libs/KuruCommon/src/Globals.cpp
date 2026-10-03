#include "Kuru/Common/Globals.hpp"

#include "Kuru/Common/Pattern.hpp"

namespace Kuru::Common {

namespace {

std::mutex gGlobalsMutex;
RocketLeagueGlobals gGlobals;

[[nodiscard]] RocketLeagueGlobals MakeGlobals(const GlobalsValidationResult& validation) {
    RocketLeagueGlobals globals{};
    globals.GNames = validation.gNames;
    globals.GObjects = validation.gObjects;
    return globals;
}

} // namespace

RocketLeagueGlobals GetRocketLeagueGlobals() {
    std::scoped_lock lock(gGlobalsMutex);
    return gGlobals;
}

void SetRocketLeagueGlobals(const RocketLeagueGlobals& globals) {
    std::scoped_lock lock(gGlobalsMutex);
    gGlobals = globals;
}

void ClearRocketLeagueGlobals() {
    std::scoped_lock lock(gGlobalsMutex);
    gGlobals = {};
}

std::optional<RocketLeagueGlobals> TryInitializeRocketLeagueGlobalsFromGNames(
    const void* gNamesAddress,
    GlobalsValidationOptions options) {
    auto validation = ValidateRocketLeagueGlobalsFromGNames(gNamesAddress, options);
    if (!validation.valid) {
        return std::nullopt;
    }

    auto globals = MakeGlobals(validation);
    SetRocketLeagueGlobals(globals);
    return globals;
}

std::optional<RocketLeagueGlobals> TryInitializeRocketLeagueGlobalsFromPattern(
    std::string_view gNamesPattern,
    GlobalsValidationOptions options) {
    auto match = FindPatternInCurrentModule(gNamesPattern);
    if (!match.has_value()) {
        return std::nullopt;
    }

    return TryInitializeRocketLeagueGlobalsFromGNames(reinterpret_cast<const void*>(*match), options);
}

} // namespace Kuru::Common
