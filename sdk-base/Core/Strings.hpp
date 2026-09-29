#pragma once

#include "Containers.hpp"

#include <cwchar>
#include <cstddef>
#include <string>
#include <string_view>

namespace Kuru::SDK {

struct FString : public TArray<wchar_t> {
    FString() = default;
    FString(const FString& other) = default;
    FString(FString&& other) noexcept = default;
    explicit FString(const wchar_t* value);
    explicit FString(std::wstring_view value);
    FString(std::int32_t count, const wchar_t* value);

    FString& operator=(const FString& other) = default;
    FString& operator=(FString&& other) noexcept = default;
    FString& operator=(const wchar_t* value);
    FString& operator=(std::wstring_view value);

    static FString Create(const wchar_t* value);
    static FString Create(std::wstring_view value);
    static FString FromAnsi(std::string_view value);
    static FString FromUtf8(std::string_view value);

    [[nodiscard]] bool IsValid() const noexcept {
        return Data != nullptr && Count > 0;
    }

    [[nodiscard]] std::wstring ToWString() const {
        if (!IsValid()) {
            return {};
        }

        const auto length = Count > 0 && Data[Count - 1] == L'\0' ? Count - 1 : Count;
        return std::wstring(Data, Data + length);
    }

    [[nodiscard]] std::string ToAnsi() const;
    [[nodiscard]] std::string ToUtf8() const;

    [[nodiscard]] std::wstring_view View() const noexcept {
        if (!IsValid()) {
            return {};
        }

        const auto length = Count > 0 && Data[Count - 1] == L'\0' ? Count - 1 : Count;
        return std::wstring_view(Data, static_cast<std::size_t>(length));
    }

    [[nodiscard]] const wchar_t* c_str() const noexcept { return Data ? Data : L""; }
    [[nodiscard]] const wchar_t* operator*() const noexcept { return c_str(); }
    [[nodiscard]] std::int32_t Len() const noexcept { return IsValid() ? Count - 1 : 0; }
    [[nodiscard]] bool Empty() const noexcept { return Len() <= 0; }
    [[nodiscard]] bool IsEmpty() const noexcept { return Empty(); }

    [[nodiscard]] wchar_t& operator[](std::int32_t index) noexcept { return Data[index]; }
    [[nodiscard]] const wchar_t& operator[](std::int32_t index) const noexcept { return Data[index]; }

    [[nodiscard]] bool Equals(std::wstring_view other, bool caseSensitive = true) const;
    [[nodiscard]] bool StartsWith(std::wstring_view prefix, bool caseSensitive = true) const;
    [[nodiscard]] bool EndsWith(std::wstring_view suffix, bool caseSensitive = true) const;
    [[nodiscard]] bool Contains(std::wstring_view needle, bool caseSensitive = true) const;
    [[nodiscard]] std::int32_t InStr(std::wstring_view needle, bool searchFromEnd = false, bool caseSensitive = true,
        std::int32_t startPosition = 0) const;
    [[nodiscard]] FString Left(std::int32_t count) const;
    [[nodiscard]] FString Right(std::int32_t count) const;
    [[nodiscard]] FString Mid(std::int32_t start, std::int32_t count = -1) const;
    [[nodiscard]] FString Trim() const;
    [[nodiscard]] FString TrimTrailing() const;
    [[nodiscard]] FString ToUpper() const;
    [[nodiscard]] FString ToLower() const;
    [[nodiscard]] FString Replace(std::wstring_view from, std::wstring_view to, bool caseSensitive = true) const;
    [[nodiscard]] std::int32_t ParseIntoArray(TArray<FString>& outArray, const wchar_t* delimiter,
        bool cullEmpty = true) const;

    FString& operator+=(const FString& other);
    FString& operator+=(const wchar_t* other);
    FString& operator+=(wchar_t character);
    FString& operator*=(const FString& path);
    FString& operator*=(const wchar_t* path);
    [[nodiscard]] FString operator+(const FString& other) const;
    [[nodiscard]] FString operator+(const wchar_t* other) const;
    [[nodiscard]] FString operator*(const FString& path) const;
    [[nodiscard]] FString operator*(const wchar_t* path) const;

    void Assign(std::wstring_view value);
    void Clear() noexcept;
    void Reserve(std::int32_t capacity);
    void AppendChar(wchar_t character);
    void AppendString(std::wstring_view value);

    [[nodiscard]] friend bool operator==(const FString& lhs, const FString& rhs) {
        return lhs.View() == rhs.View();
    }

    [[nodiscard]] friend bool operator!=(const FString& lhs, const FString& rhs) {
        return !(lhs == rhs);
    }

    [[nodiscard]] friend bool operator<(const FString& lhs, const FString& rhs) {
        return lhs.View() < rhs.View();
    }
};

struct FStringNoInit : public FString {
    FStringNoInit() = default;
};

struct FFilename : public FString {
    using FString::FString;
};

struct FStringOutputDevice : public FString {
    using FString::FString;
};

static_assert(sizeof(FString) == 0x10);
static_assert(sizeof(FStringNoInit) == 0x10);
static_assert(sizeof(FFilename) == 0x10);

} // namespace Kuru::SDK
