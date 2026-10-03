#include <Core/Strings.hpp>

#include <Core/Runtime.hpp>

#include <windows.h>

#include <algorithm>
#include <cwctype>

namespace Kuru::SDK {
namespace {
[[nodiscard]] std::wstring Widen(std::string_view value) {
    std::wstring result;
    result.reserve(value.size());
    for (char ch : value) {
        result.push_back(static_cast<unsigned char>(ch));
    }
    return result;
}

[[nodiscard]] std::wstring_view NullSafeView(const wchar_t* value) {
    return value ? std::wstring_view(value) : std::wstring_view();
}

[[nodiscard]] std::wstring LowerCopy(std::wstring_view value) {
    std::wstring result(value);
    std::transform(result.begin(), result.end(), result.begin(), [](wchar_t ch) {
        return static_cast<wchar_t>(std::towlower(ch));
    });
    return result;
}
}

FString::FString(const wchar_t* value) { Assign(NullSafeView(value)); }
FString::FString(std::wstring_view value) { Assign(value); }
FString::FString(std::int32_t count, const wchar_t* value) { Assign(value ? std::wstring_view(value, count) : std::wstring_view()); }

FString& FString::operator=(const wchar_t* value) {
    Assign(NullSafeView(value));
    return *this;
}

FString& FString::operator=(std::wstring_view value) {
    Assign(value);
    return *this;
}

FString FString::Create(const wchar_t* value) { return FString(value); }
FString FString::Create(std::wstring_view value) { return FString(value); }
FString FString::FromAnsi(std::string_view value) { return FString(Widen(value)); }

FString FString::FromUtf8(std::string_view value) {
    if (value.empty()) {
        return FString();
    }
    const int len = MultiByteToWideChar(CP_UTF8, 0, value.data(), static_cast<int>(value.size()), nullptr, 0);
    if (len <= 0) {
        return FString();
    }
    std::wstring wide(static_cast<std::size_t>(len), L'\0');
    MultiByteToWideChar(CP_UTF8, 0, value.data(), static_cast<int>(value.size()), wide.data(), len);
    return FString(wide);
}

std::string FString::ToAnsi() const {
    std::string result;
    for (wchar_t ch : View()) {
        result.push_back(ch <= 0x7F ? static_cast<char>(ch) : '?');
    }
    return result;
}

std::string FString::ToUtf8() const {
    const std::wstring_view view = View();
    if (view.empty()) {
        return {};
    }
    const int len =
        WideCharToMultiByte(CP_UTF8, 0, view.data(), static_cast<int>(view.size()), nullptr, 0, nullptr, nullptr);
    if (len <= 0) {
        return {};
    }
    std::string result(static_cast<std::size_t>(len), '\0');
    WideCharToMultiByte(CP_UTF8, 0, view.data(), static_cast<int>(view.size()), result.data(), len, nullptr, nullptr);
    return result;
}

bool FString::Equals(std::wstring_view other, bool caseSensitive) const {
    return caseSensitive ? View() == other : LowerCopy(View()) == LowerCopy(other);
}

bool FString::StartsWith(std::wstring_view prefix, bool caseSensitive) const {
    const auto value = caseSensitive ? std::wstring(View()) : LowerCopy(View());
    const auto test = caseSensitive ? std::wstring(prefix) : LowerCopy(prefix);
    return value.rfind(test, 0) == 0;
}

bool FString::EndsWith(std::wstring_view suffix, bool caseSensitive) const {
    const auto value = caseSensitive ? std::wstring(View()) : LowerCopy(View());
    const auto test = caseSensitive ? std::wstring(suffix) : LowerCopy(suffix);
    return value.size() >= test.size() && value.compare(value.size() - test.size(), test.size(), test) == 0;
}

bool FString::Contains(std::wstring_view needle, bool caseSensitive) const {
    return InStr(needle, false, caseSensitive) != -1;
}

std::int32_t FString::InStr(std::wstring_view needle, bool searchFromEnd, bool caseSensitive, std::int32_t startPosition) const {
    const auto value = caseSensitive ? std::wstring(View()) : LowerCopy(View());
    const auto test = caseSensitive ? std::wstring(needle) : LowerCopy(needle);
    const std::size_t start = startPosition < 0 ? 0 : static_cast<std::size_t>(startPosition);
    const auto pos = searchFromEnd ? value.rfind(test) : value.find(test, start);
    return pos == std::wstring::npos ? -1 : static_cast<std::int32_t>(pos);
}

FString FString::Left(std::int32_t count) const {
    const auto value = View();
    if (count <= 0) {
        return FString();
    }
    return FString(value.substr(0, std::min<std::size_t>(value.size(), static_cast<std::size_t>(count))));
}

FString FString::Right(std::int32_t count) const {
    const auto value = View();
    if (count <= 0) {
        return FString();
    }
    const auto n = std::min<std::size_t>(value.size(), static_cast<std::size_t>(count));
    return FString(value.substr(value.size() - n));
}

FString FString::Mid(std::int32_t start, std::int32_t count) const {
    const auto value = View();
    if (start < 0 || static_cast<std::size_t>(start) >= value.size()) {
        return FString();
    }
    const auto length = count < 0 ? std::wstring_view::npos : static_cast<std::size_t>(count);
    return FString(value.substr(static_cast<std::size_t>(start), length));
}

FString FString::Trim() const {
    auto value = View();
    while (!value.empty() && std::iswspace(value.front())) {
        value.remove_prefix(1);
    }
    while (!value.empty() && std::iswspace(value.back())) {
        value.remove_suffix(1);
    }
    return FString(value);
}

FString FString::TrimTrailing() const {
    auto value = View();
    while (!value.empty() && std::iswspace(value.back())) {
        value.remove_suffix(1);
    }
    return FString(value);
}

FString FString::ToUpper() const {
    std::wstring result(View());
    std::transform(result.begin(), result.end(), result.begin(), [](wchar_t ch) {
        return static_cast<wchar_t>(std::towupper(ch));
    });
    return FString(result);
}

FString FString::ToLower() const {
    return FString(LowerCopy(View()));
}

FString FString::Replace(std::wstring_view from, std::wstring_view to, bool caseSensitive) const {
    std::wstring result(View());
    const std::wstring search = caseSensitive ? std::wstring(from) : LowerCopy(from);
    std::size_t pos = 0;
    while (pos < result.size()) {
        const std::wstring haystack = caseSensitive ? result : LowerCopy(result);
        pos = haystack.find(search, pos);
        if (pos == std::wstring::npos) {
            break;
        }
        result.replace(pos, from.size(), to);
        pos += to.size();
    }
    return FString(result);
}

std::int32_t FString::ParseIntoArray(TArray<FString>& outArray, const wchar_t* delimiter, bool cullEmpty) const {
    const std::wstring delim = delimiter ? delimiter : L"";
    if (delim.empty()) {
        outArray.Add(*this);
        return outArray.Count;
    }
    std::wstring value(View());
    std::size_t start = 0;
    while (start <= value.size()) {
        const auto pos = value.find(delim, start);
        const auto part = value.substr(start, pos == std::wstring::npos ? std::wstring::npos : pos - start);
        if (!part.empty() || !cullEmpty) {
            outArray.Add(FString(part));
        }
        if (pos == std::wstring::npos) {
            break;
        }
        start = pos + delim.size();
    }
    return outArray.Count;
}

FString& FString::operator+=(const FString& other) {
    AppendString(other.View());
    return *this;
}

FString& FString::operator+=(const wchar_t* other) {
    AppendString(NullSafeView(other));
    return *this;
}

FString& FString::operator+=(wchar_t character) {
    AppendChar(character);
    return *this;
}

FString& FString::operator*=(const FString& path) {
    return (*this *= path.c_str());
}

FString& FString::operator*=(const wchar_t* path) {
    if (!Empty() && View().back() != L'/' && View().back() != L'\\') {
        AppendChar(L'/');
    }
    AppendString(NullSafeView(path));
    return *this;
}

FString FString::operator+(const FString& other) const {
    FString result(*this);
    result += other;
    return result;
}

FString FString::operator+(const wchar_t* other) const {
    FString result(*this);
    result += other;
    return result;
}

FString FString::operator*(const FString& path) const {
    FString result(*this);
    result *= path;
    return result;
}

FString FString::operator*(const wchar_t* path) const {
    FString result(*this);
    result *= path;
    return result;
}

void FString::Assign(std::wstring_view value) {
    const auto count = static_cast<std::int32_t>(value.size() + 1);
    auto* newData = static_cast<wchar_t*>(Allocate(static_cast<std::uint32_t>(sizeof(wchar_t) * count), alignof(wchar_t)));
    if (!newData) {
        Clear();
        return;
    }
    std::copy(value.begin(), value.end(), newData);
    newData[value.size()] = L'\0';
    Data = newData;
    Count = count;
    Max = count;
}

void FString::Clear() noexcept {
    Data = nullptr;
    Count = 0;
    Max = 0;
}

void FString::Reserve(std::int32_t capacity) {
    TArray<wchar_t>::Reserve(capacity);
}

void FString::AppendChar(wchar_t character) {
    const auto value = ToWString();
    Assign(value + std::wstring(1, character));
}

void FString::AppendString(std::wstring_view value) {
    std::wstring result = ToWString();
    result.append(value);
    Assign(result);
}

} // namespace Kuru::SDK
