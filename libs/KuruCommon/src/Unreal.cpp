#include "Kuru/Common/Unreal.hpp"

#include "Kuru/Common/Memory.hpp"

#include <algorithm>
#include <cwchar>
#include <string>

#include <windows.h>

namespace Kuru::Common {

namespace {

constexpr std::uintptr_t GObjectsFromGNamesOffset = 0x48;
constexpr std::ptrdiff_t FNameEntryWideNameOffset = 0x18;
constexpr std::size_t MaxNameChars = 1024;

[[nodiscard]] bool LooksLikePointer(const void* pointer) noexcept {
    const auto value = reinterpret_cast<std::uintptr_t>(pointer);
    return value >= 0x10000ULL && value <= 0x00007FFFFFFFFFFFULL;
}

// SEH-guarded raw copy of a null-terminated wide string.
[[nodiscard]] int SafeCopyWideName(const wchar_t* source, wchar_t* destination, int maxChars) noexcept {
    __try {
        for (int index = 0; index < maxChars; ++index) {
            const wchar_t ch = source[index];
            destination[index] = ch;
            if (ch == L'\0') {
                return index;
            }
        }
        return maxChars;
    } __except (EXCEPTION_EXECUTE_HANDLER) {
        return -1;
    }
}

[[nodiscard]] bool IsPlausibleNameString(const std::wstring& value) {
    if (value.empty() || value.size() >= MaxNameChars) {
        return false;
    }

    for (wchar_t ch : value) {
        if (ch == L'\0') {
            break;
        }
        if (ch < 0x20 || ch > 0x7E) {
            // RL object/name table identifiers should be mostly ASCII. Keep this conservative for validation.
            return false;
        }
    }

    return true;
}

} // namespace

std::optional<std::string> TryReadFNameEntryString(const TArray<void*>* gNames, FName name) {
    if (!LooksLikeTArray(gNames, {.minCount = 1, .maxCount = 1'000'000}) || name.Index < 0 || name.Index >= gNames->Count) {
        return std::nullopt;
    }

    void* entry = gNames->Data[name.Index];
    if (!LooksLikePointer(entry)) {
        return std::nullopt;
    }

    const auto* wideName = reinterpret_cast<const wchar_t*>(reinterpret_cast<const std::uint8_t*>(entry) + FNameEntryWideNameOffset);

    wchar_t buffer[MaxNameChars];
    const int count = SafeCopyWideName(wideName, buffer, static_cast<int>(MaxNameChars) - 1);
    if (count < 0) {
        return std::nullopt;
    }

    const std::wstring value(buffer, buffer + count);
    if (!IsPlausibleNameString(value)) {
        return std::nullopt;
    }

    return std::string(value.begin(), value.end());
}

bool LooksLikeUObject(const UObjectPrefix* object, const TArray<void*>* gNames) {
    if (!LooksLikePointer(object) || !IsReadableMemory(object, sizeof(UObjectPrefix))) {
        return false;
    }

    if (!object->VfTableObject || !IsReadableMemory(object->VfTableObject, sizeof(void*))) {
        return false;
    }

    if (object->Name.Index < 0 || !TryReadFNameEntryString(gNames, object->Name).has_value()) {
        return false;
    }

    // Class can be null for a few damaged/transient reads, but valid sampled global objects should usually have it.
    if (object->Class && (!LooksLikePointer(object->Class) || !IsReadableMemory(object->Class, sizeof(UObjectPrefix)))) {
        return false;
    }

    if (object->Outer && (!LooksLikePointer(object->Outer) || !IsReadableMemory(object->Outer, sizeof(UObjectPrefix)))) {
        return false;
    }

    return true;
}

GlobalsValidationResult ValidateRocketLeagueGlobalsFromGNames(const void* gNamesAddress, GlobalsValidationOptions options) {
    GlobalsValidationResult result{};
    result.gNames = reinterpret_cast<const TArray<void*>*>(gNamesAddress);
    result.gObjects = reinterpret_cast<const TArray<UObjectPrefix*>*>(
        reinterpret_cast<std::uintptr_t>(gNamesAddress) + GObjectsFromGNamesOffset);

    if (!LooksLikeTArray(result.gNames, {.minCount = options.minNameCount, .maxCount = options.maxNameCount})) {
        result.failureReason = "GNames candidate is not a sane TArray";
        return result;
    }

    if (!LooksLikeTArray(result.gObjects, {.minCount = options.minObjectCount, .maxCount = options.maxObjectCount})) {
        result.failureReason = "GObjects candidate at GNames + 0x48 is not a sane TArray";
        return result;
    }

    const auto count = result.gObjects->Count;
    const auto samples = std::max(1, options.objectSampleCount);
    const auto stride = std::max(1, count / samples);

    for (std::int32_t index = 0; index < count && result.validObjectSamples < samples; index += stride) {
        UObjectPrefix* object = result.gObjects->Data[index];
        if (!object) {
            continue;
        }
        if (LooksLikeUObject(object, result.gNames)) {
            ++result.validObjectSamples;
        }
    }

    if (result.validObjectSamples < options.minimumValidObjectSamples) {
        result.failureReason = "GObjects TArray did not contain enough valid UObject samples";
        return result;
    }

    result.valid = true;
    return result;
}

} // namespace Kuru::Common
