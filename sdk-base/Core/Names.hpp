#pragma once

#include "Strings.hpp"

#include <cstddef>
#include <cstdint>
#include <string>
#include <string_view>

namespace Kuru::SDK {

constexpr std::int32_t NAME_SIZE = 1024;
constexpr std::ptrdiff_t FNAME_ENTRY_WIDE_NAME_OFFSET = 0x18;

enum EFindName : std::uint8_t {
    FNAME_Find = 0,
    FNAME_Add = 1,
    FNAME_Replace_Not_Safe_For_Threading = 2,
};

enum EName : std::int32_t {
    NAME_None = 0,
};

struct FNameEntry;

struct FName {
    std::int32_t Index = 0;
    std::int32_t Number = 0;

    constexpr FName() = default;
    constexpr FName(std::int32_t index, std::int32_t number = 0) noexcept
        : Index(index), Number(number) {}
    constexpr FName(EName name) noexcept : Index(static_cast<std::int32_t>(name)), Number(0) {}
    FName(const wchar_t* name, EFindName findType = FNAME_Add);
    FName(const char* name, EFindName findType = FNAME_Add);
    FName(std::string_view name, EFindName findType = FNAME_Add);
    FName(std::wstring_view name, EFindName findType = FNAME_Add);

    [[nodiscard]] constexpr bool IsNone() const noexcept {
        return Index == 0 && Number == 0;
    }

    [[nodiscard]] constexpr bool IsValid() const noexcept { return Index >= 0; }
    [[nodiscard]] constexpr std::int32_t GetIndex() const noexcept { return Index; }
    [[nodiscard]] constexpr std::int32_t GetNumber() const noexcept { return Number; }

    [[nodiscard]] std::string ToString() const;
    void ToString(FString& out) const;
    void AppendString(FString& out) const;
    [[nodiscard]] std::string GetPlainNameString() const;
    [[nodiscard]] std::string GetNumberedNameString() const;
    [[nodiscard]] FNameEntry* GetDisplayNameEntry() const;
    [[nodiscard]] FNameEntry* GetComparisonNameEntry() const;
    [[nodiscard]] std::uint32_t GetComparisonIndex() const noexcept { return static_cast<std::uint32_t>(Index); }
    [[nodiscard]] std::uint32_t GetDisplayIndex() const noexcept { return static_cast<std::uint32_t>(Index); }
    [[nodiscard]] static FName Find(std::string_view name, bool add = false);
    [[nodiscard]] static TArray<FNameEntry*>* Names();
    [[nodiscard]] static std::uint32_t GetTypeHash(FName name) noexcept {
        return static_cast<std::uint32_t>(name.Index) ^ static_cast<std::uint32_t>(name.Number << 16);
    }

    [[nodiscard]] friend constexpr bool operator==(FName lhs, FName rhs) noexcept {
        return lhs.Index == rhs.Index && lhs.Number == rhs.Number;
    }

    [[nodiscard]] friend constexpr bool operator!=(FName lhs, FName rhs) noexcept {
        return !(lhs == rhs);
    }

    [[nodiscard]] friend constexpr bool operator<(FName lhs, FName rhs) noexcept {
        return lhs.Index < rhs.Index || (lhs.Index == rhs.Index && lhs.Number < rhs.Number);
    }
};

struct FNameEntry {
    std::uint64_t Flags = 0;
    std::int32_t Index = 0;
    std::uint8_t Pad_000C[0x0C]{};
    wchar_t Name[NAME_SIZE]{};

    [[nodiscard]] const wchar_t* GetWideName() const noexcept { return Name; }
    [[nodiscard]] const wchar_t* GetName() const noexcept { return Name; }
    void AppendNameToString(FString& out) const;
};

struct FNameEntryId {
    std::int32_t Value = 0;
};

static_assert(sizeof(FName) == 0x8);
static_assert(offsetof(FName, Index) == 0x0);
static_assert(offsetof(FName, Number) == 0x4);
static_assert(sizeof(FNameEntry) == 0x818);
static_assert(offsetof(FNameEntry, Name) == FNAME_ENTRY_WIDE_NAME_OFFSET);
static_assert(sizeof(FNameEntryId) == 0x4);

} // namespace Kuru::SDK
