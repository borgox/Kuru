#include <Kuru/Common/DumpSchema.hpp>
#include <Kuru/Common/Globals.hpp>
#include <Kuru/Common/Memory.hpp>
#include <Kuru/Common/Pattern.hpp>
#include <Kuru/Common/TArray.hpp>
#include <Kuru/Common/Unreal.hpp>

#include <windows.h>
#include <psapi.h>

#include <algorithm>
#include <cstddef>
#include <cstdint>
#include <cstdio>
#include <cstring>
#include <fstream>
#include <optional>
#include <set>
#include <sstream>
#include <string>
#include <string_view>
#include <vector>

namespace {

// Proven by Kuru-dump for the current tracked RL build family. GNamesPattern is shared (Kuru::Common).
constexpr const char* GMallocPattern = "48 89 0D ?? ?? ?? ?? 48 8B 01 FF 50 60";
constexpr const char* GPsyonixBuildIDPattern = "4C 8B 0D ?? ?? ?? ?? 4C 8D 05 ?? ?? ?? ?? BA F8 02 00 00";
constexpr const char* BuildDatePattern = "48 8B 0D ?? ?? ?? ?? E8 ?? ?? ?? ?? 48 8B 3D ?? ?? ?? ?? 48 85 FF 74";
constexpr int MaxOuterDepth = 32;
constexpr int MaxChildFields = 20'000;

using Kuru::Common::FName;
using Kuru::Common::TArray;
using Kuru::Common::UObjectPrefix;

struct UFieldRuntime : UObjectPrefix {
    UFieldRuntime* Next;
    std::uint8_t Pad_0068[0x08];
};

struct UPropertyRuntime;
struct FStringRuntime : TArray<wchar_t> {};

struct UStructRuntime : UFieldRuntime {
    std::uint8_t Unknown_0070[0x10];
    UStructRuntime* SuperStruct;
    UFieldRuntime* Children;
    std::int32_t PropertiesSize;
    std::uint32_t StructPad_0094;
    TArray<std::uint8_t> Script;
    std::int32_t TextPos;
    std::int32_t Line;
    std::int32_t MinAlignment;
    std::uint32_t StructPad_00B4;
    std::uint8_t RLPad_00B8[0x50];
    UPropertyRuntime* RefLink;
    UPropertyRuntime* PropertyLink;
    UPropertyRuntime* ConstructorLink;
    TArray<UObjectPrefix*> ScriptObjectReferences;
};

struct UFunctionRuntime : UStructRuntime {
    std::uint64_t FunctionFlags;
    std::uint16_t iNative;
    std::uint16_t RepOffset;
    FName FriendlyName;
    std::uint8_t OperPrecedence;
    std::uint8_t NumParms;
    std::uint16_t ParmsSize;
    std::uint16_t ReturnValueOffset;
    std::uint8_t FunctionPad_014A[0x06];
    void* FirstStructWithDefaults;
    void* Func;
};

struct UStateRuntime : UStructRuntime {
    std::uint8_t Unknown_0130[0x60];
};

struct FRepRecordRuntime {
    UPropertyRuntime* Property;
    std::int32_t Index;
    std::uint32_t RepPad_000C;
};

struct UClassRuntime : UStateRuntime {
    std::uint32_t ClassFlags;
    std::uint32_t ClassCastFlags;
    std::uint8_t Unknown_0198[0x50];
    std::uint64_t RLClassCastFlags;
    UClassRuntime* ClassWithin;
    FName ClassConfigName;
    TArray<FRepRecordRuntime> ClassReps;
    TArray<UFieldRuntime*> NetFields;
    std::uint8_t Unknown_0220[0x198];
};

struct UPropertyRuntime : UFieldRuntime {
    std::int32_t ArrayDim;
    std::int32_t ElementSize;
    std::uint64_t PropertyFlags;
    std::uint16_t RepOffset;
    std::uint16_t RepIndex;
    std::uint32_t PropertyPad_0084;
    FName Category;
    UObjectPrefix* ArraySizeEnum;
    std::int32_t Offset;
    std::uint32_t PropertyPad_009C;
    UPropertyRuntime* PropertyLinkNext;
    UPropertyRuntime* ConstructorLinkNext;
    UPropertyRuntime* NextRef;
    FStringRuntime RLPropertyName;
};

struct UObjectPropertyRuntime : UPropertyRuntime { UClassRuntime* PropertyClass; std::uint8_t RLPad_00D0[0x08]; };
struct UClassPropertyRuntime : UObjectPropertyRuntime { UClassRuntime* MetaClass; };
struct UBytePropertyRuntime : UPropertyRuntime { UObjectPrefix* Enum; };
struct UBoolPropertyRuntime : UPropertyRuntime { std::uint64_t BitMask; };
struct UArrayPropertyRuntime : UPropertyRuntime { UPropertyRuntime* Inner; };
struct UMapPropertyRuntime : UPropertyRuntime { UPropertyRuntime* Key; UPropertyRuntime* Value; };
struct UStructPropertyRuntime : UPropertyRuntime { UStructRuntime* Struct; };
struct UInterfacePropertyRuntime : UPropertyRuntime { UClassRuntime* InterfaceClass; std::uint8_t RLPad_00D0[0x08]; };
struct UEnumRuntime : UFieldRuntime { TArray<FName> Names; };

static_assert(sizeof(UObjectPrefix) == 0x60);
static_assert(sizeof(UFieldRuntime) == 0x70);
static_assert(sizeof(UStructRuntime) == 0x130);
static_assert(sizeof(UFunctionRuntime) == 0x160);
static_assert(sizeof(UClassRuntime) == 0x3B8);
static_assert(sizeof(UPropertyRuntime) == 0xC8);
static_assert(sizeof(UObjectPropertyRuntime) == 0xD8);
static_assert(sizeof(UClassPropertyRuntime) == 0xE0);
static_assert(sizeof(UInterfacePropertyRuntime) == 0xD8);

std::ofstream gLog;
std::string gOutputDirectory;

// The reflection model built in-memory by the walk and handed straight to the generator
Kuru::Common::DumpSchema::DumpDocument gDocument;
bool gDocumentValid = false;

void Log(const std::string& message) {
    const std::string line = "[KuruSDKGen] " + message + "\n";
    OutputDebugStringA(line.c_str());
    if (gLog.is_open()) {
        gLog << line;
        gLog.flush();
    }
}

std::string Hex(std::uintptr_t value) {
    char buffer[2 + 16 + 1];
    char* cursor = buffer + sizeof(buffer);
    *--cursor = '\0';
    static constexpr char kDigits[] = "0123456789ABCDEF";
    if (value == 0) {
        *--cursor = '0';
    } else {
        while (value != 0) {
            *--cursor = kDigits[value & 0xF];
            value >>= 4;
        }
    }
    *--cursor = 'x';
    *--cursor = '0';
    return std::string(cursor);
}

bool Readable(const void* ptr, std::size_t size) {
    return Kuru::Common::IsReadableMemory(ptr, size);
}

template <typename T>
bool ReadableObject(const T* ptr) {
    return Readable(ptr, sizeof(T));
}

void OpenLog() {
    gLog.open(gOutputDirectory + "\\KuruSDKGen.log", std::ios::app);
}

std::string DirectoryFromPath(std::string_view path) {
    const auto slash = path.find_last_of("\\/");
    if (slash == std::string_view::npos) {
        return {};
    }

    return std::string(path.substr(0, slash));
}

bool InitializeOutputDirectory(HMODULE module) {
    char path[MAX_PATH]{};
    const DWORD len = GetModuleFileNameA(module, path, MAX_PATH);
    if (len == 0 || len >= MAX_PATH) {
        return false;
    }

    gOutputDirectory = DirectoryFromPath(std::string_view(path, len));
    return !gOutputDirectory.empty();
}

void NotifyUser(const char* title, const std::string& message, UINT icon) {
    MessageBoxA(nullptr, message.c_str(), title, MB_OK | icon | MB_SETFOREGROUND);
}

DWORD FailMainThread(const std::string& reason) {
    Log(reason);
    NotifyUser("KuruSDKGen failed",
               reason + "\n\nOutput directory:\n" + gOutputDirectory +
                   "\n\nCheck KuruSDKGen.log if it was created.",
               MB_ICONERROR);
    return 0;
}

std::optional<std::uintptr_t> ResolveRipRelativeGlobalSlot(
    std::string_view pattern,
    std::ptrdiff_t displacementOffset = 3,
    std::ptrdiff_t instructionLength = 7) {
    const auto match = Kuru::Common::FindPatternInCurrentModule(pattern);
    if (!match.has_value()) {
        return std::nullopt;
    }

    const auto displacementAddress = *match + static_cast<std::uintptr_t>(displacementOffset);
    if (!Readable(reinterpret_cast<const void*>(displacementAddress), sizeof(std::int32_t))) {
        return std::nullopt;
    }

    const auto displacement = *reinterpret_cast<const std::int32_t*>(displacementAddress);
    const auto nextInstruction = *match + static_cast<std::uintptr_t>(instructionLength);
    const auto slot = static_cast<std::uintptr_t>(
        static_cast<std::int64_t>(nextInstruction) + static_cast<std::int64_t>(displacement));

    return slot;
}

std::uintptr_t TryReadPointerSlot(std::uintptr_t slot) {
    if (slot == 0 || !Readable(reinterpret_cast<const void*>(slot), sizeof(void*))) {
        return 0;
    }

    return reinterpret_cast<std::uintptr_t>(*reinterpret_cast<void* const*>(slot));
}

std::string TryReadAnsiCString(const char* value, std::size_t maxLen) {
    if (!value || !Readable(value, sizeof(char))) {
        return {};
    }

    std::string result;
    result.reserve(std::min<std::size_t>(maxLen, 64));
    for (std::size_t index = 0; index < maxLen; ++index) {
        if (!Readable(value + index, sizeof(char))) {
            return {};
        }
        const char ch = value[index];
        if (ch == '\0' || ch == '\r' || ch == '\n') {
            break;
        }
        if (static_cast<unsigned char>(ch) < 0x20) {
            return {};
        }
        result.push_back(ch);
    }
    return result;
}

std::string TryReadWideCString(const wchar_t* value, std::size_t maxLen) {
    if (!value || !Readable(value, sizeof(wchar_t))) {
        return {};
    }

    std::string result;
    result.reserve(std::min<std::size_t>(maxLen, 64));
    for (std::size_t index = 0; index < maxLen; ++index) {
        if (!Readable(value + index, sizeof(wchar_t))) {
            return {};
        }

        const wchar_t ch = value[index];
        if (ch == L'\0' || ch == L'\r' || ch == L'\n') {
            break;
        }
        if (ch < 0x20 || ch > 0x7E) {
            return {};
        }
        result.push_back(static_cast<char>(ch));
    }

    return result;
}

bool IsPlausibleMetadataString(std::string_view value) {
    if (value.empty()) {
        return false;
    }

    for (char ch : value) {
        const auto byte = static_cast<unsigned char>(ch);
        if (byte < 0x20 || byte > 0x7E) {
            return false;
        }
    }

    return true;
}

std::optional<std::string> ScanModuleStringAfter(const char* needle, std::size_t maxLen) {
    HMODULE module = GetModuleHandleA(nullptr);
    if (!module) {
        return std::nullopt;
    }

    MODULEINFO info{};
    if (!GetModuleInformation(GetCurrentProcess(), module, &info, sizeof(info))) {
        return std::nullopt;
    }

    const auto* base = reinterpret_cast<const std::uint8_t*>(info.lpBaseOfDll);
    const std::size_t size = static_cast<std::size_t>(info.SizeOfImage);
    const std::size_t needleLen = std::strlen(needle);

    for (std::size_t i = 0; i + needleLen + 1 < size; ++i) {
        if (std::memcmp(base + i, needle, needleLen) != 0) {
            continue;
        }

        std::string value;
        const char* p = reinterpret_cast<const char*>(base + i + needleLen);
        for (std::size_t k = 0; k < maxLen && p[k] && p[k] != '\r' && p[k] != '\n'; ++k) {
            value.push_back(p[k]);
        }
        if (!value.empty()) {
            return value;
        }
    }

    return std::nullopt;
}

std::string ResolveBuildDateValue(std::uintptr_t buildDateSlot) {
    const auto pointer = TryReadPointerSlot(buildDateSlot);
    if (pointer == 0) {
        return {};
    }

    const std::string value = TryReadAnsiCString(reinterpret_cast<const char*>(pointer), 128);
    return IsPlausibleMetadataString(value) ? value : std::string{};
}

std::string ResolvePsyonixBuildIDValue(std::uintptr_t buildIdSlot) {
    const auto pointer = TryReadPointerSlot(buildIdSlot);
    if (pointer == 0) {
        return {};
    }

    const std::string value = TryReadWideCString(reinterpret_cast<const wchar_t*>(pointer), 64);
    return IsPlausibleMetadataString(value) ? value : std::string{};
}

std::optional<std::string> ExtractJsonStringValue(std::string_view content, std::string_view key) {
    const auto keyPos = content.find(key);
    if (keyPos == std::string_view::npos) {
        return std::nullopt;
    }

    const auto colon = content.find(':', keyPos + key.size());
    if (colon == std::string_view::npos) {
        return std::nullopt;
    }

    const auto quoteBegin = content.find('"', colon + 1);
    if (quoteBegin == std::string_view::npos) {
        return std::nullopt;
    }

    const auto quoteEnd = content.find('"', quoteBegin + 1);
    if (quoteEnd == std::string_view::npos) {
        return std::nullopt;
    }

    return std::string(content.substr(quoteBegin + 1, quoteEnd - quoteBegin - 1));
}

std::string BuildLabel() {
    char programData[MAX_PATH]{};
    if (!GetEnvironmentVariableA("ProgramData", programData, MAX_PATH)) {
        return "unknown";
    }

    const std::string manifestDir = std::string(programData) + "\\Epic\\EpicGamesLauncher\\Data\\Manifests";
    WIN32_FIND_DATAA findData{};
    HANDLE find = FindFirstFileA((manifestDir + "\\*.item").c_str(), &findData);
    if (find == INVALID_HANDLE_VALUE) {
        return "unknown";
    }

    do {
        const std::string path = manifestDir + "\\" + findData.cFileName;
        HANDLE file = CreateFileA(path.c_str(), GENERIC_READ, FILE_SHARE_READ, nullptr, OPEN_EXISTING, 0, nullptr);
        if (file == INVALID_HANDLE_VALUE) {
            continue;
        }

        const DWORD size = GetFileSize(file, nullptr);
        if (size == INVALID_FILE_SIZE || size > 2 * 1024 * 1024) {
            CloseHandle(file);
            continue;
        }

        std::string content(size, '\0');
        DWORD read = 0;
        const BOOL ok = ReadFile(file, content.data(), size, &read, nullptr);
        CloseHandle(file);
        if (!ok) {
            continue;
        }
        content.resize(read);

        const auto appName = ExtractJsonStringValue(content, "\"AppName\"");
        if (!appName.has_value() || *appName != "Sugar") {
            continue;
        }

        const auto version = ExtractJsonStringValue(content, "\"AppVersionString\"");
        if (version.has_value() && !version->empty()) {
            FindClose(find);
            return *version;
        }
    } while (FindNextFileA(find, &findData));

    FindClose(find);
    return "unknown";
}

std::uint32_t BuildIdFromLabel(std::string_view label) {
    constexpr std::string_view marker = "-CL-";
    const auto cl = label.find(marker);
    if (cl == std::string_view::npos) {
        return 0;
    }

    std::uint32_t id = 0;
    if (std::sscanf(std::string(label.substr(cl + marker.size())).c_str(), "%u", &id) == 1) {
        return id;
    }

    return 0;
}

std::uint32_t ParsePsyonixBuildId(std::string_view value) {
    std::uint32_t id = 0;
    if (std::sscanf(std::string(value).c_str(), "%u", &id) == 1) {
        return id;
    }
    return 0;
}

std::uint32_t PsyonixBuildId(const Kuru::Common::RocketLeagueGlobals& globals) {
    if (!globals.PsyonixBuildID.empty()) {
        if (const auto id = ParsePsyonixBuildId(globals.PsyonixBuildID); id != 0) {
            return id;
        }
    }

    auto value = ScanModuleStringAfter("GPsyonixBuildID ", 32);
    if (value.has_value()) {
        if (const auto id = ParsePsyonixBuildId(*value); id != 0) {
            return id;
        }
    }
    return BuildIdFromLabel(BuildLabel());
}

std::string PsyonixBuildIdString(const Kuru::Common::RocketLeagueGlobals& globals) {
    if (!globals.PsyonixBuildID.empty()) {
        return globals.PsyonixBuildID;
    }

    if (const auto id = PsyonixBuildId(globals); id != 0) {
        return std::to_string(id);
    }

    return "unknown";
}

std::string NameFromFName(const Kuru::Common::RocketLeagueGlobals& globals, FName name) {
    static std::vector<std::string> cache;
    const int index = name.Index;
    if (index >= 0 && globals.GNames) {
        const int count = globals.GNames->Count;
        if (index < count) {
            if (static_cast<int>(cache.size()) < count) {
                cache.resize(static_cast<std::size_t>(count));
            }
            std::string& slot = cache[static_cast<std::size_t>(index)];
            if (slot.empty()) {
                slot = Kuru::Common::TryReadFNameEntryString(globals.GNames, name).value_or("None");
            }
            return slot;
        }
    }
    return Kuru::Common::TryReadFNameEntryString(globals.GNames, name).value_or("None");
}

std::string ObjectName(const Kuru::Common::RocketLeagueGlobals& globals, const UObjectPrefix* object) {
    if (!object || !ReadableObject(object)) {
        return "None";
    }
    return NameFromFName(globals, object->Name);
}

std::string ObjectFullName(const Kuru::Common::RocketLeagueGlobals& globals, const UObjectPrefix* object) {
    if (!object || !ReadableObject(object)) {
        return "None";
    }

    std::string fullName = ObjectName(globals, object);
    const auto* outer = object->Outer;
    int depth = 0;
    while (outer && depth++ < MaxOuterDepth) {
        if (!ReadableObject(outer)) {
            break;
        }
        fullName = ObjectName(globals, outer) + "." + fullName;
        outer = outer->Outer;
    }
    return fullName;
}

std::string ClassName(const Kuru::Common::RocketLeagueGlobals& globals, const UObjectPrefix* object) {
    if (!object || !ReadableObject(object) || !object->Class || !ReadableObject(object->Class)) {
        return "None";
    }
    return ObjectName(globals, object->Class);
}

bool IsPropertyClassName(const std::string& className) {
    return className.find("Property") != std::string::npos;
}

std::string PropertyReferenceName(const Kuru::Common::RocketLeagueGlobals& globals, const UObjectPrefix* object) {
    return object ? ObjectFullName(globals, object) : "None";
}

std::vector<const UFieldRuntime*> CollectChildren(const UStructRuntime* strct) {
    std::vector<const UFieldRuntime*> fields;
    if (!ReadableObject(strct)) {
        return fields;
    }

    std::set<const UFieldRuntime*> seen;
    const UFieldRuntime* field = strct->Children;
    while (field && fields.size() < MaxChildFields) {
        if (!ReadableObject(field) || !seen.insert(field).second) {
            break;
        }
        fields.push_back(field);
        field = field->Next;
    }
    return fields;
}

std::string PackageNameFromFullName(const std::string& fullName) {
    const auto dot = fullName.find('.');
    if (dot == std::string::npos || dot == 0) {
        return "<unknown>";
    }
    return fullName.substr(0, dot);
}

void LogSampleObjects(const Kuru::Common::RocketLeagueGlobals& globals) {
    if (!globals.GObjects || globals.GObjects->Count <= 0) {
        return;
    }

    Log("Sample objects:");
    const int stride = globals.GObjects->Count > 8 ? globals.GObjects->Count / 8 : 1;
    int printed = 0;

    for (int index = 0; index < globals.GObjects->Count && printed < 8; index += stride) {
        const auto* object = globals.GObjects->Data[index];
        if (!object) {
            continue;
        }

        auto name = Kuru::Common::TryReadFNameEntryString(globals.GNames, object->Name);
        if (!name.has_value()) {
            continue;
        }

        std::ostringstream line;
        line << "  [" << index << "] " << *name << " @ " << Hex(reinterpret_cast<std::uintptr_t>(object));
        Log(line.str());
        ++printed;
    }
}

// --- In-memory model builders ---------------------------------------------------------------------
Kuru::Common::DumpSchema::PropertyRecord BuildPropertyRecord(
    const Kuru::Common::RocketLeagueGlobals& globals, const UPropertyRuntime* prop, unsigned depth = 0) {
    Kuru::Common::DumpSchema::PropertyRecord record;
    const std::string kind = ClassName(globals, prop);
    record.name = ObjectName(globals, prop);
    record.kind = kind;
    record.flags = prop->PropertyFlags;
    record.offset = prop->Offset;
    record.elementSize = prop->ElementSize;
    record.arrayDim = prop->ArrayDim;

    if (kind == "ObjectProperty" || kind == "ComponentProperty") {
        const auto* typed = reinterpret_cast<const UObjectPropertyRuntime*>(prop);
        if (ReadableObject(typed)) record.propertyClass = PropertyReferenceName(globals, typed->PropertyClass);
    } else if (kind == "ClassProperty") {
        const auto* typed = reinterpret_cast<const UClassPropertyRuntime*>(prop);
        if (ReadableObject(typed)) {
            record.propertyClass = PropertyReferenceName(globals, typed->PropertyClass);
            record.metaClass = PropertyReferenceName(globals, typed->MetaClass);
        }
    } else if (kind == "ByteProperty") {
        const auto* typed = reinterpret_cast<const UBytePropertyRuntime*>(prop);
        if (ReadableObject(typed)) record.enumName = PropertyReferenceName(globals, typed->Enum);
    } else if (kind == "BoolProperty") {
        const auto* typed = reinterpret_cast<const UBoolPropertyRuntime*>(prop);
        if (ReadableObject(typed)) record.bitMask = Hex(typed->BitMask);
    } else if (kind == "ArrayProperty") {
        const auto* typed = reinterpret_cast<const UArrayPropertyRuntime*>(prop);
        if (ReadableObject(typed)) {
            record.inner = PropertyReferenceName(globals, typed->Inner);
            if (depth < 16 && typed->Inner && ReadableObject(typed->Inner)) {
                record.innerProperties.push_back(BuildPropertyRecord(globals, typed->Inner, depth + 1));
            }
        }
    } else if (kind == "MapProperty") {
        const auto* typed = reinterpret_cast<const UMapPropertyRuntime*>(prop);
        if (ReadableObject(typed)) {
            record.key = PropertyReferenceName(globals, typed->Key);
            record.value = PropertyReferenceName(globals, typed->Value);
        }
    } else if (kind == "StructProperty") {
        const auto* typed = reinterpret_cast<const UStructPropertyRuntime*>(prop);
        if (ReadableObject(typed)) record.structName = PropertyReferenceName(globals, typed->Struct);
    } else if (kind == "InterfaceProperty") {
        const auto* typed = reinterpret_cast<const UInterfacePropertyRuntime*>(prop);
        if (ReadableObject(typed)) record.interfaceClass = PropertyReferenceName(globals, typed->InterfaceClass);
    }
    return record;
}

Kuru::Common::DumpSchema::FunctionRecord BuildFunctionRecord(
    const Kuru::Common::RocketLeagueGlobals& globals, const UFunctionRuntime* fn) {
    Kuru::Common::DumpSchema::FunctionRecord record;
    record.fullName = ObjectFullName(globals, fn);
    record.flags = fn->FunctionFlags;
    record.parmsSize = fn->ParmsSize;
    record.numParms = fn->NumParms;
    record.returnValueOffset = fn->ReturnValueOffset;
    for (const UFieldRuntime* field : CollectChildren(reinterpret_cast<const UStructRuntime*>(fn))) {
        if (IsPropertyClassName(ClassName(globals, field))) {
            const auto* prop = reinterpret_cast<const UPropertyRuntime*>(field);
            if (ReadableObject(prop)) record.params.push_back(BuildPropertyRecord(globals, prop));
        }
    }
    return record;
}

Kuru::Common::DumpSchema::DumpDocument BuildDumpDocument(const Kuru::Common::RocketLeagueGlobals& globals) {
    Kuru::Common::DumpSchema::DumpDocument document;
    document.game = "Rocket League";

    int typeCount = 0;
    for (int index = 0; index < globals.GObjects->Count; ++index) {
        if (index != 0 && (index % 10000) == 0) {
            Log("Build progress index=" + std::to_string(index) + "/" + std::to_string(globals.GObjects->Count) +
                " types=" + std::to_string(typeCount));
        }

        const auto* object = globals.GObjects->Data[index];
        if (!Kuru::Common::LooksLikeUObject(object, globals.GNames)) {
            continue;
        }

        const std::string className = ClassName(globals, object);
        if (className != "Class" && className != "ScriptStruct" && className != "Enum") {
            continue;  // Only reflected types feed the generator.
        }

        Kuru::Common::DumpSchema::ObjectRecord record;
        record.address = reinterpret_cast<std::uintptr_t>(object);
        record.name = ObjectName(globals, object);
        record.fullName = ObjectFullName(globals, object);
        record.packageName = PackageNameFromFullName(record.fullName);
        record.className = className;

        if (className == "Class" || className == "ScriptStruct") {
            const auto* strct = reinterpret_cast<const UStructRuntime*>(object);
            if (ReadableObject(strct)) {
                record.superName = strct->SuperStruct ? ObjectFullName(globals, strct->SuperStruct) : "None";
                record.propertySize = strct->PropertiesSize;
                record.minAlignment = strct->MinAlignment;
                for (const UFieldRuntime* field : CollectChildren(strct)) {
                    const std::string fieldKind = ClassName(globals, field);
                    if (IsPropertyClassName(fieldKind)) {
                        const auto* prop = reinterpret_cast<const UPropertyRuntime*>(field);
                        if (ReadableObject(prop)) record.properties.push_back(BuildPropertyRecord(globals, prop));
                    } else if (fieldKind == "Function") {
                        const auto* fn = reinterpret_cast<const UFunctionRuntime*>(field);
                        if (ReadableObject(fn)) record.functions.push_back(BuildFunctionRecord(globals, fn));
                    }
                }
            }
        } else {  // Enum
            const auto* enumObj = reinterpret_cast<const UEnumRuntime*>(object);
            if (ReadableObject(enumObj) && enumObj->Names.Count >= 0 && enumObj->Names.Count <= 4096 &&
                enumObj->Names.Data &&
                Readable(enumObj->Names.Data, sizeof(FName) * static_cast<std::size_t>(enumObj->Names.Count))) {
                for (int i = 0; i < enumObj->Names.Count; ++i) {
                    record.enumNames.push_back(NameFromFName(globals, enumObj->Names.Data[i]));
                }
            }
        }

        document.objects.push_back(std::move(record));
        ++typeCount;
    }

    Log("Built reflection model types=" + std::to_string(typeCount) + " from " +
        std::to_string(globals.GObjects->Count) + " objects");
    return document;
}

DWORD WINAPI MainThread(LPVOID parameter) {
    const auto selfModule = reinterpret_cast<HMODULE>(parameter);
    if (!InitializeOutputDirectory(selfModule)) {
        gOutputDirectory = ".";
        NotifyUser("KuruSDKGen failed",
                   "Could not resolve the KuruSDKGen DLL path, so the dumper cannot choose an output directory.",
                   MB_ICONERROR);
        return 0;
    }

    OpenLog();
    Log("Output directory: " + gOutputDirectory);
    Log("Injected, scanning for GNames using known-good Kuru-dump pattern");

    const auto match = Kuru::Common::FindPatternInCurrentModule(Kuru::Common::GNamesPattern);
    if (!match.has_value()) {
        return FailMainThread("GNames pattern not found");
    }

    Log("GNames candidate found at " + Hex(*match));

    Kuru::Common::GlobalsValidationOptions options{};
    options.minNameCount = 5'000;
    options.minObjectCount = 10'000;
    options.objectSampleCount = 128;
    options.minimumValidObjectSamples = 16;

    const auto validation = Kuru::Common::ValidateRocketLeagueGlobalsFromGNames(
        reinterpret_cast<const void*>(*match),
        options);

    if (!validation.valid) {
        return FailMainThread("Global validation failed: " + validation.failureReason);
    }

    Kuru::Common::RocketLeagueGlobals globals{};
    globals.GNames = validation.gNames;
    globals.GObjects = validation.gObjects;

    if (const auto gMallocSlot = ResolveRipRelativeGlobalSlot(GMallocPattern); gMallocSlot.has_value()) {
        globals.GMallocAddress = *gMallocSlot;
        globals.GMallocValue = TryReadPointerSlot(*gMallocSlot);
        Log("GMalloc slot resolved at " + Hex(globals.GMallocAddress) + " value=" + Hex(globals.GMallocValue));
    } else {
        Log("GMalloc pattern not resolved; continuing without GMalloc metadata");
    }

    if (const auto buildIdSlot = ResolveRipRelativeGlobalSlot(GPsyonixBuildIDPattern); buildIdSlot.has_value()) {
        globals.PsyonixBuildIDAddress = *buildIdSlot;
        globals.PsyonixBuildID = ResolvePsyonixBuildIDValue(*buildIdSlot);
        Log("GPsyonixBuildID slot resolved at " + Hex(globals.PsyonixBuildIDAddress) +
            " value=" + (globals.PsyonixBuildID.empty() ? std::string("unknown") : globals.PsyonixBuildID));
    } else {
        globals.PsyonixBuildID = PsyonixBuildIdString(globals);
        Log("GPsyonixBuildID pattern not resolved; using fallback value=" + globals.PsyonixBuildID);
    }

    if (const auto buildDateSlot = ResolveRipRelativeGlobalSlot(BuildDatePattern, 15, 19); buildDateSlot.has_value()) {
        globals.BuildDateAddress = *buildDateSlot;
        globals.BuildDate = ResolveBuildDateValue(*buildDateSlot);
        Log("BuildDate slot resolved at " + Hex(globals.BuildDateAddress) +
            " value=" + (globals.BuildDate.empty() ? std::string("unknown") : globals.BuildDate));
    } else {
        Log("BuildDate pattern not resolved; continuing without BuildDate metadata");
    }

    Kuru::Common::SetRocketLeagueGlobals(globals);

    Log("GNames validated at " + Hex(reinterpret_cast<std::uintptr_t>(globals.GNames)) +
        " count=" + std::to_string(globals.GNames->Count));
    Log("GObjects validated at " + Hex(reinterpret_cast<std::uintptr_t>(globals.GObjects)) +
        " count=" + std::to_string(globals.GObjects->Count));
    Log("Valid UObject samples=" + std::to_string(validation.validObjectSamples));

    LogSampleObjects(globals);

    gDocument = BuildDumpDocument(globals);
    gDocumentValid = !gDocument.objects.empty();
    if (!gDocumentValid) {
        return FailMainThread("Reflection walk produced no types");
    }
    Log("Reflection walk complete; handing model to the generator");

    return 0;
}

} // namespace

namespace Kuru::Dump {
bool Run(void* selfModule, Kuru::Common::DumpSchema::DumpDocument& outDocument, std::string& outDir) {
    gDocumentValid = false;
    MainThread(reinterpret_cast<HMODULE>(selfModule));
    outDir = gOutputDirectory;
    if (!gDocumentValid) {
        return false;
    }
    outDocument = std::move(gDocument);
    return true;
}
}  // namespace Kuru::Dump
