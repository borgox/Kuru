#include <Core/Names.hpp>

#include <Core/Runtime.hpp>

#include <sstream>

namespace Kuru::SDK {

FName::FName(const wchar_t* name, EFindName findType) {
    if (auto* constructor = GetNameConstructor()) {
        constructor(this, name, findType);
        return;
    }
    *this = Find(FString(name).ToAnsi(), findType == FNAME_Add);
}

FName::FName(const char* name, EFindName findType)
    : FName(name ? std::string_view(name) : std::string_view(), findType) {}

FName::FName(std::string_view name, EFindName findType)
    : FName(FString::FromAnsi(name).View(), findType) {}

FName::FName(std::wstring_view name, EFindName findType) {
    if (auto* constructor = GetNameConstructor()) {
        const std::wstring copy(name);
        constructor(this, copy.c_str(), findType);
        return;
    }
    *this = Find(FString(name).ToAnsi(), findType == FNAME_Add);
}

std::string FName::ToString() const {
    if (FNameEntry* entry = GetDisplayNameEntry()) {
        FString out;
        entry->AppendNameToString(out);
        std::string plain = out.ToAnsi();
        if (Number > 0) {
            plain += "_" + std::to_string(Number - 1);
        }
        return plain;
    }
    return {};
}

void FName::ToString(FString& out) const {
    out.Assign(FString::FromAnsi(ToString()).View());
}

void FName::AppendString(FString& out) const {
    out += FString::FromAnsi(ToString());
}

std::string FName::GetPlainNameString() const {
    if (FNameEntry* entry = GetDisplayNameEntry()) {
        return FString(entry->GetWideName()).ToAnsi();
    }
    return {};
}

std::string FName::GetNumberedNameString() const {
    return ToString();
}

FNameEntry* FName::GetDisplayNameEntry() const {
    auto* names = Names();
    if (!names || Index < 0 || Index >= names->Count) {
        return nullptr;
    }
    return names->Data[Index];
}

FNameEntry* FName::GetComparisonNameEntry() const {
    return GetDisplayNameEntry();
}

FName FName::Find(std::string_view name, bool add) {
    auto* names = Names();
    if (!names) {
        return FName(NAME_None);
    }
    for (std::int32_t index = 0; index < names->Count; ++index) {
        const FNameEntry* entry = names->Data[index];
        if (!entry) {
            continue;
        }
        if (FString(entry->GetWideName()).ToAnsi() == name) {
            return FName(index);
        }
    }
    if (add) {
        return FName(FString::FromAnsi(name).View(), FNAME_Add);
    }
    return FName(NAME_None);
}

TArray<FNameEntry*>* FName::Names() {
    return GetGlobalNames();
}

void FNameEntry::AppendNameToString(FString& out) const {
    out += Name;
}

} // namespace Kuru::SDK
