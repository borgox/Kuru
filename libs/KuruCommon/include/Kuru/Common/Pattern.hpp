#pragma once

#include <cstddef>
#include <cstdint>
#include <optional>
#include <string>
#include <string_view>
#include <vector>

namespace Kuru::Common {

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
