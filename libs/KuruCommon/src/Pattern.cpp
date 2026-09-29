#include "Kuru/Common/Pattern.hpp"

#include "Kuru/Common/Memory.hpp"

#include <algorithm>
#include <charconv>
#include <cctype>
#include <sstream>
#include <vector>

#include <windows.h>
#include <psapi.h>

namespace Kuru::Common {

namespace {

[[nodiscard]] bool HasScannableProtection(DWORD protect) noexcept {
    return (protect & PAGE_GUARD) == 0 && (protect & PAGE_NOACCESS) == 0;
}

[[nodiscard]] bool IsWildcardToken(std::string_view token) noexcept {
    return token == "?" || token == "??";
}

[[nodiscard]] std::optional<std::uint8_t> ParseHexByte(std::string_view token) noexcept {
    if (token.size() != 2) {
        return std::nullopt;
    }

    unsigned int value = 0;
    const auto* first = token.data();
    const auto* last = token.data() + token.size();
    const auto result = std::from_chars(first, last, value, 16);
    if (result.ec != std::errc{} || result.ptr != last || value > 0xFF) {
        return std::nullopt;
    }

    return static_cast<std::uint8_t>(value);
}

[[nodiscard]] bool MatchesAt(const std::uint8_t* data, const Pattern& pattern) noexcept {
    for (std::size_t index = 0; index < pattern.bytes.size(); ++index) {
        const auto& byte = pattern.bytes[index];
        if (!byte.wildcard && data[index] != byte.value) {
            return false;
        }
    }
    return true;
}

} // namespace

std::optional<Pattern> ParsePattern(std::string_view text) {
    std::istringstream stream{std::string(text)};
    std::string token;
    Pattern pattern;

    while (stream >> token) {
        if (IsWildcardToken(token)) {
            pattern.bytes.push_back({0, true});
            continue;
        }

        auto value = ParseHexByte(token);
        if (!value.has_value()) {
            return std::nullopt;
        }
        pattern.bytes.push_back({*value, false});
    }

    if (pattern.empty()) {
        return std::nullopt;
    }

    return pattern;
}

std::optional<std::uintptr_t> FindPatternInRange(std::uintptr_t begin, std::size_t size, const Pattern& pattern) {
    if (pattern.empty() || size < pattern.size()) {
        return std::nullopt;
    }

    const HANDLE process = GetCurrentProcess();
    const auto end = begin + size;
    if (end < begin) {
        return std::nullopt;
    }

    std::uintptr_t current = begin;
    while (current < end) {
        MEMORY_BASIC_INFORMATION mbi{};
        if (VirtualQueryEx(process, reinterpret_cast<LPCVOID>(current), &mbi, sizeof(mbi)) == 0) {
            break;
        }

        const auto regionBegin = std::max(current, reinterpret_cast<std::uintptr_t>(mbi.BaseAddress));
        const auto regionEnd = std::min(
            end,
            reinterpret_cast<std::uintptr_t>(mbi.BaseAddress) + static_cast<std::uintptr_t>(mbi.RegionSize));

        if (regionEnd > regionBegin && mbi.State == MEM_COMMIT && HasScannableProtection(mbi.Protect)) {
            const auto regionSize = static_cast<std::size_t>(regionEnd - regionBegin);
            std::vector<std::uint8_t> buffer(regionSize);
            SIZE_T bytesRead = 0;

            if (ReadProcessMemory(process,
                                  reinterpret_cast<LPCVOID>(regionBegin),
                                  buffer.data(),
                                  regionSize,
                                  &bytesRead) &&
                bytesRead >= pattern.size()) {
                const auto lastStart = bytesRead - pattern.size();
                for (std::size_t offset = 0; offset <= lastStart; ++offset) {
                    if (MatchesAt(buffer.data() + offset, pattern)) {
                        return regionBegin + offset;
                    }
                }
            }
        }

        const auto next = reinterpret_cast<std::uintptr_t>(mbi.BaseAddress) + static_cast<std::uintptr_t>(mbi.RegionSize);
        if (next <= current) {
            break;
        }
        current = next;
    }

    return std::nullopt;
}

std::optional<std::uintptr_t> FindPatternInCurrentModule(const Pattern& pattern) {
    HMODULE module = GetModuleHandleW(nullptr);
    if (!module) {
        return std::nullopt;
    }

    MODULEINFO info{};
    if (!GetModuleInformation(GetCurrentProcess(), module, &info, sizeof(info))) {
        return std::nullopt;
    }

    return FindPatternInRange(reinterpret_cast<std::uintptr_t>(info.lpBaseOfDll), static_cast<std::size_t>(info.SizeOfImage), pattern);
}

std::optional<std::uintptr_t> FindPatternInCurrentModule(std::string_view patternText) {
    auto pattern = ParsePattern(patternText);
    if (!pattern.has_value()) {
        return std::nullopt;
    }
    return FindPatternInCurrentModule(*pattern);
}

} // namespace Kuru::Common
