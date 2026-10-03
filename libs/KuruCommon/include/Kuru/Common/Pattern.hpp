#pragma once

#include <cstddef>
#include <cstdint>
#include <optional>
#include <string>
#include <string_view>
#include <vector>

namespace Kuru::Common {

// The Rocket League GNames byte pattern, shared so the loader and the SDK generator don't each carry
// their own copy (GObjects follows at GNames + 0x48).
inline constexpr std::string_view GNamesPattern = "?? ?? ?? ?? ?? ?? 00 00 ?? ?? ?? ?? 35 25 02 00";

struct PatternByte {
    std::uint8_t value = 0;
    bool wildcard = false;
};

struct Pattern {
    std::vector<PatternByte> bytes;

    [[nodiscard]] bool empty() const noexcept { return bytes.empty(); }
    [[nodiscard]] std::size_t size() const noexcept { return bytes.size(); }
};

[[nodiscard]] std::optional<Pattern> ParsePattern(std::string_view text);
[[nodiscard]] std::optional<std::uintptr_t> FindPatternInRange(std::uintptr_t begin, std::size_t size, const Pattern& pattern);
[[nodiscard]] std::optional<std::uintptr_t> FindPatternInCurrentModule(const Pattern& pattern);
[[nodiscard]] std::optional<std::uintptr_t> FindPatternInCurrentModule(std::string_view patternText);

} // namespace Kuru::Common
