#include <Kuru/Common/DumpSchema.hpp>
#include <Kuru/Common/Globals.hpp>
#include <Kuru/Common/Memory.hpp>
#include <Kuru/Common/Pattern.hpp>
#include <Kuru/Common/TArray.hpp>
#include <Kuru/Common/Unreal.hpp>

#include <windows.h>
#include <psapi.h>
#include <winver.h>

#include <algorithm>
#include <chrono>
#include <cstddef>
#include <cstdint>
#include <cstdio>
#include <cstring>
#include <ctime>
#include <fstream>
#include <iomanip>
#include <map>
#include <optional>
#include <set>
#include <sstream>
#include <string>
#include <string_view>
#include <vector>

namespace {

// Proven by Kuru-dump for the current tracked RL build family.
constexpr const char* GNamesPattern = "?? ?? ?? ?? ?? ?? 00 00 ?? ?? ?? ?? 35 25 02 00";
constexpr const char* GMallocPattern = "48 89 0D ?? ?? ?? ?? 48 8B 01 FF 50 60";
constexpr const char* GPsyonixBuildIDPattern = "4C 8B 0D ?? ?? ?? ?? 4C 8D 05 ?? ?? ?? ?? BA F8 02 00 00";
constexpr const char* BuildDatePattern = "48 8B 0D ?? ?? ?? ?? E8 ?? ?? ?? ?? 48 8B 3D ?? ?? ?? ?? 48 85 FF 74";
constexpr int MaxOuterDepth = 32;
constexpr int MaxChildFields = 20'000;
constexpr int MaxScriptBytes = 500'000;
constexpr std::uint64_t CPF_Parm = 0x80ULL;
constexpr std::uint64_t CPF_OutParm = 0x100ULL;
constexpr std::uint64_t CPF_ReturnParm = 0x400ULL;

constexpr const char* KnownPackages[] = {
    "Core",
    "Engine",
    "IpDrv",
    "XAudio2",
    "GFxUI",
    "AkAudio",
    "WinDrv",
    "OnlineSubsystemEOS",
    "ProjectX",
    "TAGame",
};

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
struct UDelegatePropertyRuntime : UPropertyRuntime { UFunctionRuntime* Function; };
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

struct DumpSummary {
    int writtenObjects = 0;
    int structLikeObjects = 0;
    int classObjects = 0;
    int scriptStructObjects = 0;
    int stateObjects = 0;
    int propertyObjects = 0;
    int functionObjects = 0;
    int enumObjects = 0;
    int defaultObjects = 0;
    int objectsWithBytecode = 0;
    std::map<std::string, int> packageCounts;
    std::map<std::string, int> classCounts;
    std::map<std::string, int> propertyKindCounts;
    std::vector<std::string> knownPackagesPresent;
    std::vector<std::string> knownPackagesMissing;
};

std::ofstream gLog;
std::string gOutputDirectory;

void Log(const std::string& message) {
    const std::string line = "[KuruDump] " + message + "\n";
    OutputDebugStringA(line.c_str());
    if (gLog.is_open()) {
        gLog << line;
        gLog.flush();
    }
}

std::string Hex(std::uintptr_t value) {
    std::ostringstream out;
    out << "0x" << std::hex << std::uppercase << value << std::nouppercase << std::dec;
    return out.str();
}

std::string Json(std::string_view value) {
    return Kuru::Common::DumpSchema::JsonEscape(value);
}

bool Readable(const void* ptr, std::size_t size) {
    return Kuru::Common::IsReadableMemory(ptr, size);
}

template <typename T>
bool ReadableObject(const T* ptr) {
    return Readable(ptr, sizeof(T));
}

void OpenLog() {
    gLog.open(gOutputDirectory + "\\KuruDump.log", std::ios::app);
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

std::string OutputPath(std::string_view fileName) {
    return gOutputDirectory + "\\" + std::string(fileName);
}

void NotifyUser(const char* title, const std::string& message, UINT icon) {
    MessageBoxA(nullptr, message.c_str(), title, MB_OK | icon | MB_SETFOREGROUND);
}

DWORD FailMainThread(const std::string& reason) {
    Log(reason);
    NotifyUser("KuruDump failed",
               reason + "\n\nOutput directory:\n" + gOutputDirectory +
                   "\n\nCheck KuruDump.log if it was created.",
               MB_ICONERROR);
    return 0;
}

std::string TimestampUtc() {
    const auto now = std::chrono::system_clock::now();
    const std::time_t time = std::chrono::system_clock::to_time_t(now);
    std::tm utc{};
    gmtime_s(&utc, &time);
    std::ostringstream out;
    out << std::put_time(&utc, "%Y-%m-%dT%H:%M:%SZ");
    return out.str();
}

std::string ExePath() {
    char path[MAX_PATH]{};
    const DWORD len = GetModuleFileNameA(nullptr, path, MAX_PATH);
    return len > 0 ? std::string(path, len) : std::string("unknown");
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

std::string UE3VersionString() {
    return ScanModuleStringAfter("Init: Version: ", 64).value_or("unknown");
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

std::string ExeFileVersion() {
    const std::string path = ExePath();
    DWORD dummy = 0;
    const DWORD size = GetFileVersionInfoSizeA(path.c_str(), &dummy);
    if (size == 0) {
        return "unknown";
    }

    std::vector<std::uint8_t> buffer(size);
    if (!GetFileVersionInfoA(path.c_str(), 0, size, buffer.data())) {
        return "unknown";
    }

    VS_FIXEDFILEINFO* info = nullptr;
    UINT infoLen = 0;
    if (!VerQueryValueA(buffer.data(), "\\", reinterpret_cast<void**>(&info), &infoLen) || !info) {
        return "unknown";
    }

    std::ostringstream version;
    version << HIWORD(info->dwFileVersionMS) << "."
            << LOWORD(info->dwFileVersionMS) << "."
            << HIWORD(info->dwFileVersionLS) << "."
            << LOWORD(info->dwFileVersionLS);
    return version.str();
}

std::string NameFromFName(const Kuru::Common::RocketLeagueGlobals& globals, FName name) {
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

bool IsStructLikeClassName(const std::string& className) {
    return className == "Class" || className == "ScriptStruct" || className == "Function" || className == "State";
}

std::string PropertyReferenceName(const Kuru::Common::RocketLeagueGlobals& globals, const UObjectPrefix* object) {
    return object ? ObjectFullName(globals, object) : "None";
}

bool StartsWith(std::string_view value, std::string_view prefix) {
    return value.size() >= prefix.size() && value.substr(0, prefix.size()) == prefix;
}

std::string NarrowStringFromWide(const wchar_t* value, int count) {
    if (!value || count <= 0 || count > 4096 || !Readable(value, static_cast<std::size_t>(count) * sizeof(wchar_t))) {
        return {};
    }

    std::wstring wide(value, value + count);
    if (!wide.empty() && wide.back() == L'\0') {
        wide.pop_back();
    }
    if (wide.empty()) {
        return {};
    }

    const int required = WideCharToMultiByte(
        CP_UTF8,
        0,
        wide.data(),
        static_cast<int>(wide.size()),
        nullptr,
        0,
        nullptr,
        nullptr);
    if (required <= 0) {
        return {};
    }

    std::string utf8(static_cast<std::size_t>(required), '\0');
    const int written = WideCharToMultiByte(
        CP_UTF8,
        0,
        wide.data(),
        static_cast<int>(wide.size()),
        utf8.data(),
        required,
        nullptr,
        nullptr);
    if (written <= 0) {
        return {};
    }
    return utf8;
}

std::string ScriptHex(const TArray<std::uint8_t>& script) {
    if (!script.Data || script.Count <= 0 || script.Count > MaxScriptBytes || !Readable(script.Data, static_cast<std::size_t>(script.Count))) {
        return {};
    }

    std::ostringstream out;
    out << std::hex << std::uppercase << std::setfill('0');
    for (std::int32_t i = 0; i < script.Count; ++i) {
        out << std::setw(2) << static_cast<int>(script.Data[i]);
    }
    return out.str();
}

void WritePropertyJson(std::ostream& out, const Kuru::Common::RocketLeagueGlobals& globals, const UPropertyRuntime* prop) {
    const std::string kind = ClassName(globals, prop);
    out << "{";
    out << "\"name\":\"" << Json(ObjectName(globals, prop)) << "\",";
    out << "\"full_name\":\"" << Json(ObjectFullName(globals, prop)) << "\",";
    out << "\"kind\":\"" << Json(kind) << "\",";
    out << "\"offset\":" << prop->Offset << ",";
    out << "\"element_size\":" << prop->ElementSize << ",";
    out << "\"array_dim\":" << prop->ArrayDim << ",";
    out << "\"flags\":\"" << Hex(prop->PropertyFlags) << "\",";
    out << "\"rep_offset\":" << prop->RepOffset << ",";
    out << "\"rep_index\":" << prop->RepIndex;

    if (kind == "ObjectProperty" || kind == "ComponentProperty") {
        const auto* typed = reinterpret_cast<const UObjectPropertyRuntime*>(prop);
        if (ReadableObject(typed)) {
            out << ",\"property_class\":\"" << Json(PropertyReferenceName(globals, typed->PropertyClass)) << "\"";
        }
    } else if (kind == "ClassProperty") {
        const auto* typed = reinterpret_cast<const UClassPropertyRuntime*>(prop);
        if (ReadableObject(typed)) {
            out << ",\"property_class\":\"" << Json(PropertyReferenceName(globals, typed->PropertyClass)) << "\"";
            out << ",\"meta_class\":\"" << Json(PropertyReferenceName(globals, typed->MetaClass)) << "\"";
        }
    } else if (kind == "ByteProperty") {
        const auto* typed = reinterpret_cast<const UBytePropertyRuntime*>(prop);
        if (ReadableObject(typed)) {
            out << ",\"enum\":\"" << Json(PropertyReferenceName(globals, typed->Enum)) << "\"";
        }
    } else if (kind == "BoolProperty") {
        const auto* typed = reinterpret_cast<const UBoolPropertyRuntime*>(prop);
        if (ReadableObject(typed)) {
            out << ",\"bit_mask\":\"" << Hex(typed->BitMask) << "\"";
        }
    } else if (kind == "ArrayProperty") {
        const auto* typed = reinterpret_cast<const UArrayPropertyRuntime*>(prop);
        if (ReadableObject(typed)) {
            out << ",\"inner\":\"" << Json(PropertyReferenceName(globals, typed->Inner)) << "\"";
            out << ",\"inner_kind\":\"" << Json(ClassName(globals, typed->Inner)) << "\"";
        }
    } else if (kind == "MapProperty") {
        const auto* typed = reinterpret_cast<const UMapPropertyRuntime*>(prop);
        if (ReadableObject(typed)) {
            out << ",\"key\":\"" << Json(PropertyReferenceName(globals, typed->Key)) << "\"";
            out << ",\"key_kind\":\"" << Json(ClassName(globals, typed->Key)) << "\"";
            out << ",\"value\":\"" << Json(PropertyReferenceName(globals, typed->Value)) << "\"";
            out << ",\"value_kind\":\"" << Json(ClassName(globals, typed->Value)) << "\"";
        }
    } else if (kind == "StructProperty") {
        const auto* typed = reinterpret_cast<const UStructPropertyRuntime*>(prop);
        if (ReadableObject(typed)) {
            out << ",\"struct\":\"" << Json(PropertyReferenceName(globals, typed->Struct)) << "\"";
        }
    } else if (kind == "DelegateProperty") {
        const auto* typed = reinterpret_cast<const UDelegatePropertyRuntime*>(prop);
        if (ReadableObject(typed)) {
            out << ",\"function\":\"" << Json(PropertyReferenceName(globals, typed->Function)) << "\"";
        }
    } else if (kind == "InterfaceProperty") {
        const auto* typed = reinterpret_cast<const UInterfacePropertyRuntime*>(prop);
        if (ReadableObject(typed)) {
            out << ",\"interface_class\":\"" << Json(PropertyReferenceName(globals, typed->InterfaceClass)) << "\"";
        }
    }

    out << "}";
}

std::vector<const UPropertyRuntime*> CollectPropertiesRecursive(const Kuru::Common::RocketLeagueGlobals& globals, const UStructRuntime* strct) {
    std::vector<const UPropertyRuntime*> properties;
    std::vector<const UStructRuntime*> chain;
    std::set<const UStructRuntime*> seenStructs;

    const UStructRuntime* current = strct;
    while (current && ReadableObject(current) && seenStructs.insert(current).second) {
        chain.push_back(current);
        current = current->SuperStruct;
    }

    std::reverse(chain.begin(), chain.end());
    for (const UStructRuntime* item : chain) {
        std::set<const UFieldRuntime*> seenFields;
        const UFieldRuntime* field = item->Children;
        int walked = 0;
        while (field && walked++ < MaxChildFields) {
            if (!ReadableObject(field) || !seenFields.insert(field).second) {
                break;
            }
            if (IsPropertyClassName(ClassName(globals, field))) {
                const auto* prop = reinterpret_cast<const UPropertyRuntime*>(field);
                if (ReadableObject(prop)) {
                    properties.push_back(prop);
                }
            }
            field = field->Next;
        }
    }

    return properties;
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

void WritePropertiesArray(std::ostream& out, const Kuru::Common::RocketLeagueGlobals& globals, const UStructRuntime* strct) {
    out << "[";
    bool first = true;
    for (const UFieldRuntime* field : CollectChildren(strct)) {
        const std::string kind = ClassName(globals, field);
        if (!IsPropertyClassName(kind)) {
            continue;
        }
        const auto* prop = reinterpret_cast<const UPropertyRuntime*>(field);
        if (!ReadableObject(prop)) {
            continue;
        }
        if (!first) {
            out << ",";
        }
        first = false;
        WritePropertyJson(out, globals, prop);
    }
    out << "]";
}

void WriteFunctionsArray(std::ostream& out, const Kuru::Common::RocketLeagueGlobals& globals, const UStructRuntime* strct) {
    out << "[";
    bool first = true;
    for (const UFieldRuntime* field : CollectChildren(strct)) {
        const std::string kind = ClassName(globals, field);
        if (kind != "Function") {
            continue;
        }
        const auto* fn = reinterpret_cast<const UFunctionRuntime*>(field);
        if (!ReadableObject(fn)) {
            continue;
        }
        if (!first) {
            out << ",";
        }
        first = false;

        const std::string script = ScriptHex(fn->Script);
        out << "{";
        out << "\"name\":\"" << Json(ObjectName(globals, fn)) << "\",";
        out << "\"full_name\":\"" << Json(ObjectFullName(globals, fn)) << "\",";
        out << "\"flags\":\"" << Hex(fn->FunctionFlags) << "\",";
        out << "\"i_native\":" << fn->iNative << ",";
        out << "\"rep_offset\":" << fn->RepOffset << ",";
        out << "\"friendly_name\":\"" << Json(NameFromFName(globals, fn->FriendlyName)) << "\",";
        out << "\"operator_precedence\":" << static_cast<int>(fn->OperPrecedence) << ",";
        out << "\"num_parms\":" << static_cast<int>(fn->NumParms) << ",";
        out << "\"parms_size\":" << fn->ParmsSize << ",";
        out << "\"return_value_offset\":" << fn->ReturnValueOffset << ",";
        out << "\"func\":\"" << Hex(reinterpret_cast<std::uintptr_t>(fn->Func)) << "\",";
        out << "\"bytecode_len\":" << fn->Script.Count << ",";
        out << "\"bytecode\":\"" << script << "\",";
        out << "\"params\":";
        WritePropertiesArray(out, globals, fn);
        out << "}";
    }
    out << "]";
}

std::optional<std::string> DefaultPropertyValue(
    const Kuru::Common::RocketLeagueGlobals& globals,
    const UObjectPrefix* object,
    const UPropertyRuntime* prop) {
    if (!ReadableObject(object) || !ReadableObject(prop) || prop->Offset < 0 || prop->ElementSize <= 0) {
        return std::nullopt;
    }

    if ((prop->PropertyFlags & (CPF_Parm | CPF_OutParm | CPF_ReturnParm)) != 0) {
        return std::nullopt;
    }

    const auto* valueAddress = reinterpret_cast<const std::uint8_t*>(object) + prop->Offset;
    if (!Readable(valueAddress, static_cast<std::size_t>(std::min(prop->ElementSize, 64)))) {
        return std::nullopt;
    }

    const std::string kind = ClassName(globals, prop);
    std::ostringstream value;

    if (kind == "BoolProperty") {
        value << ((*valueAddress) ? "true" : "false");
        return value.str();
    }
    if (kind == "IntProperty" || kind == "UIntProperty") {
        value << *reinterpret_cast<const std::int32_t*>(valueAddress);
        return value.str();
    }
    if (kind == "FloatProperty") {
        value << *reinterpret_cast<const float*>(valueAddress);
        return value.str();
    }
    if (kind == "ByteProperty") {
        const std::uint8_t raw = *valueAddress;
        const auto* byteProp = reinterpret_cast<const UBytePropertyRuntime*>(prop);
        if (ReadableObject(byteProp) && byteProp->Enum) {
            const auto* enumObj = reinterpret_cast<const UEnumRuntime*>(byteProp->Enum);
            if (ReadableObject(enumObj) && enumObj->Names.Data && raw < enumObj->Names.Count) {
                return NameFromFName(globals, enumObj->Names.Data[raw]);
            }
        }
        value << static_cast<int>(raw);
        return value.str();
    }
    if (kind == "NameProperty") {
        if (Readable(valueAddress, sizeof(FName))) {
            return NameFromFName(globals, *reinterpret_cast<const FName*>(valueAddress));
        }
        return std::nullopt;
    }
    if (kind == "StrProperty" || kind == "StringRefProperty") {
        const auto* str = reinterpret_cast<const FStringRuntime*>(valueAddress);
        if (ReadableObject(str)) {
            return NarrowStringFromWide(str->Data, str->Count);
        }
        return std::nullopt;
    }
    if (kind == "ObjectProperty" || kind == "ComponentProperty" || kind == "ClassProperty" || kind == "InterfaceProperty") {
        if (Readable(valueAddress, sizeof(UObjectPrefix*))) {
            const auto* ref = *reinterpret_cast<UObjectPrefix* const*>(valueAddress);
            return ref ? ObjectFullName(globals, ref) : std::string("None");
        }
        return std::nullopt;
    }

    if (prop->ElementSize > 0 && prop->ElementSize <= 64) {
        value << "0x" << std::hex << std::uppercase << std::setfill('0');
        for (int i = 0; i < prop->ElementSize; ++i) {
            value << std::setw(2) << static_cast<int>(valueAddress[i]);
        }
        return value.str();
    }

    return std::nullopt;
}


void WriteDefaultProperties(std::ostream& out, const Kuru::Common::RocketLeagueGlobals& globals, const UObjectPrefix* object) {
    out << "[";
    if (!object || !object->Class || !ReadableObject(object->Class)) {
        out << "]";
        return;
    }

    const auto* klass = reinterpret_cast<const UStructRuntime*>(object->Class);
    bool first = true;
    for (const UPropertyRuntime* prop : CollectPropertiesRecursive(globals, klass)) {
        auto value = DefaultPropertyValue(globals, object, prop);
        if (!value.has_value()) {
            continue;
        }
        if (!first) {
            out << ",";
        }
        first = false;
        out << "{";
        out << "\"name\":\"" << Json(ObjectName(globals, prop)) << "\",";
        out << "\"kind\":\"" << Json(ClassName(globals, prop)) << "\",";
        out << "\"offset\":" << prop->Offset << ",";
        out << "\"value\":\"" << Json(*value) << "\"";
        out << "}";
    }
    out << "]";
}

void WriteEnumMetadata(std::ostream& out, const Kuru::Common::RocketLeagueGlobals& globals, const UEnumRuntime* enumObj) {
    if (!ReadableObject(enumObj) || enumObj->Names.Count < 0 || enumObj->Names.Count > 4096 ||
        !enumObj->Names.Data || !Readable(enumObj->Names.Data, sizeof(FName) * static_cast<std::size_t>(enumObj->Names.Count))) {
        return;
    }

    out << ",\"enum_names\":[";
    for (int i = 0; i < enumObj->Names.Count; ++i) {
        if (i != 0) {
            out << ",";
        }
        out << "\"" << Json(NameFromFName(globals, enumObj->Names.Data[i])) << "\"";
    }
    out << "]";
}

void WriteNativeQwordSnapshot(
    std::ostream& out,
    const Kuru::Common::RocketLeagueGlobals& globals,
    const UObjectPrefix* object,
    std::size_t startOffset,
    std::size_t endOffset) {
    if (!ReadableObject(object) || endOffset <= startOffset) {
        return;
    }

    const auto* base = reinterpret_cast<const std::uint8_t*>(object);
    if (!Readable(base + startOffset, endOffset - startOffset)) {
        return;
    }

    out << ",\"native_qwords\":[";
    bool first = true;
    for (std::size_t offset = startOffset; offset + sizeof(std::uint64_t) <= endOffset; offset += sizeof(std::uint64_t)) {
        const auto raw = *reinterpret_cast<const std::uint64_t*>(base + offset);
        if (!first) {
            out << ",";
        }
        first = false;

        out << "{\"offset\":\"" << Hex(offset) << "\",\"value\":\"" << Hex(raw) << "\"";
        const auto* ref = reinterpret_cast<const UObjectPrefix*>(static_cast<std::uintptr_t>(raw));
        if (raw >= 0x10000ULL && raw <= 0x00007FFFFFFFFFFFULL && ReadableObject(ref) && Kuru::Common::LooksLikeUObject(ref, globals.GNames)) {
            out << ",\"object\":\"" << Json(ObjectFullName(globals, ref)) << "\"";
        }
        out << "}";
    }
    out << "]";
}

void WriteClassMetadata(std::ostream& out, const Kuru::Common::RocketLeagueGlobals& globals, const UClassRuntime* klass) {
    if (!ReadableObject(klass)) {
        return;
    }
    out << ",\"class_flags\":\"" << Hex(klass->ClassFlags) << "\"";
    const auto effectiveCastFlags = static_cast<std::uint64_t>(klass->ClassCastFlags) | klass->RLClassCastFlags;
    out << ",\"class_cast_flags\":\"" << Hex(effectiveCastFlags) << "\"";
    out << ",\"class_cast_flags_0194\":\"" << Hex(klass->ClassCastFlags) << "\"";
    out << ",\"rl_class_cast_flags_01e8\":\"" << Hex(klass->RLClassCastFlags) << "\"";
    out << ",\"class_within\":\"" << Json(PropertyReferenceName(globals, klass->ClassWithin)) << "\"";
    out << ",\"class_config_name\":\"" << Json(NameFromFName(globals, klass->ClassConfigName)) << "\"";
    out << ",\"class_reps_count\":" << (klass->ClassReps.Count >= 0 ? klass->ClassReps.Count : 0);
    out << ",\"class_reps\":[";
    if (klass->ClassReps.Count > 0 && klass->ClassReps.Count < 4096 && klass->ClassReps.Data &&
        Readable(klass->ClassReps.Data, sizeof(FRepRecordRuntime) * static_cast<std::size_t>(klass->ClassReps.Count))) {
        for (int i = 0; i < klass->ClassReps.Count; ++i) {
            if (i != 0) {
                out << ",";
            }
            const auto& rep = klass->ClassReps.Data[i];
            out << "{\"property\":\"" << Json(PropertyReferenceName(globals, rep.Property)) << "\",\"index\":" << rep.Index << "}";
        }
    }
    out << "]";
    out << ",\"net_fields_count\":" << (klass->NetFields.Count >= 0 ? klass->NetFields.Count : 0);
    out << ",\"net_fields\":[";
    if (klass->NetFields.Count > 0 && klass->NetFields.Count < 4096 && klass->NetFields.Data &&
        Readable(klass->NetFields.Data, sizeof(UFieldRuntime*) * static_cast<std::size_t>(klass->NetFields.Count))) {
        for (int i = 0; i < klass->NetFields.Count; ++i) {
            if (i != 0) {
                out << ",";
            }
            out << "\"" << Json(PropertyReferenceName(globals, klass->NetFields.Data[i])) << "\"";
        }
    }
    out << "]";
    WriteNativeQwordSnapshot(out, globals, klass, 0x190, sizeof(UClassRuntime));
}

bool IsCoreLayoutProbeObject(std::string_view fullName, std::string_view className) {
    if (!StartsWith(fullName, "Core.")) {
        return false;
    }

    static constexpr std::string_view Names[] = {
        "Core.Field",
        "Core.Struct",
        "Core.ScriptStruct",
        "Core.State",
        "Core.Class",
        "Core.Function",
        "Core.Property",
        "Core.ObjectProperty",
        "Core.ClassProperty",
        "Core.ByteProperty",
        "Core.BoolProperty",
        "Core.IntProperty",
        "Core.QWordProperty",
        "Core.SQWordProperty",
        "Core.FloatProperty",
        "Core.NameProperty",
        "Core.StrProperty",
        "Core.ArrayProperty",
        "Core.MapProperty",
        "Core.StructProperty",
        "Core.DelegateProperty",
        "Core.InterfaceProperty",
        "Core.ComponentProperty",
        "Core.Enum",
        "Core.Const",
    };

    for (std::string_view name : Names) {
        if (fullName == name) {
            return true;
        }
    }

    return className == "Property" || className.find("Property") != std::string_view::npos;
}

void WriteCoreLayoutProbe(
    std::ostream& out,
    const Kuru::Common::RocketLeagueGlobals& globals,
    const UObjectPrefix* object,
    std::string_view fullName,
    std::string_view className) {
    if (!IsCoreLayoutProbeObject(fullName, className)) {
        return;
    }

    std::size_t endOffset = sizeof(UObjectPrefix);
    if (className == "Class") {
        endOffset = sizeof(UClassRuntime);
    } else if (className == "Function") {
        endOffset = sizeof(UFunctionRuntime);
    } else if (className == "State") {
        endOffset = sizeof(UStateRuntime);
    } else if (className == "ScriptStruct" || fullName == "Core.Struct") {
        endOffset = sizeof(UStructRuntime);
    } else if (className == "Enum" || fullName == "Core.Enum") {
        endOffset = sizeof(UEnumRuntime);
    } else if (className == "Const" || fullName == "Core.Const") {
        endOffset = 0x80;
    } else if (className == "ClassProperty") {
        endOffset = sizeof(UClassPropertyRuntime);
    } else if (className == "MapProperty") {
        endOffset = sizeof(UMapPropertyRuntime);
    } else if (className == "ObjectProperty" || className == "ComponentProperty") {
        endOffset = sizeof(UObjectPropertyRuntime);
    } else if (className == "ByteProperty") {
        endOffset = sizeof(UBytePropertyRuntime);
    } else if (className == "BoolProperty") {
        endOffset = sizeof(UBoolPropertyRuntime);
    } else if (className == "ArrayProperty") {
        endOffset = sizeof(UArrayPropertyRuntime);
    } else if (className == "StructProperty") {
        endOffset = sizeof(UStructPropertyRuntime);
    } else if (className == "DelegateProperty") {
        endOffset = sizeof(UDelegatePropertyRuntime);
    } else if (className == "InterfaceProperty") {
        endOffset = sizeof(UInterfacePropertyRuntime);
    } else if (className.find("Property") != std::string_view::npos || fullName == "Core.Property") {
        endOffset = sizeof(UPropertyRuntime);
    } else if (className == "Field" || fullName == "Core.Field") {
        endOffset = sizeof(UFieldRuntime);
    }

    if (endOffset > sizeof(UObjectPrefix)) {
        WriteNativeQwordSnapshot(out, globals, object, sizeof(UObjectPrefix), endOffset);
    }
}

std::string PackageNameFromFullName(const std::string& fullName) {
    const auto dot = fullName.find('.');
    if (dot == std::string::npos || dot == 0) {
        return "<unknown>";
    }
    return fullName.substr(0, dot);
}

void WriteStringArray(std::ostream& out, const std::vector<std::string>& values) {
    out << "[";
    for (std::size_t i = 0; i < values.size(); ++i) {
        if (i != 0) {
            out << ",";
        }
        out << "\"" << Json(values[i]) << "\"";
    }
    out << "]";
}

void WriteStringIntMap(std::ostream& out, const std::map<std::string, int>& values) {
    out << "{";
    bool first = true;
    for (const auto& [key, value] : values) {
        if (!first) {
            out << ",";
        }
        first = false;
        out << "\"" << Json(key) << "\":" << value;
    }
    out << "}";
}

bool HasMapKey(const std::map<std::string, int>& values, std::string_view key) {
    return values.find(std::string(key)) != values.end();
}

void FillKnownPackagePresence(DumpSummary& summary) {
    summary.knownPackagesPresent.clear();
    summary.knownPackagesMissing.clear();

    for (const char* package : KnownPackages) {
        if (HasMapKey(summary.packageCounts, package)) {
            summary.knownPackagesPresent.push_back(package);
        } else {
            summary.knownPackagesMissing.push_back(package);
        }
    }
}

void WriteSummaryJson(std::ostream& out, const DumpSummary& summary, std::string_view indent) {
    out << indent << "\"written_object_count\": " << summary.writtenObjects << ",\n";
    out << indent << "\"struct_like_objects\": " << summary.structLikeObjects << ",\n";
    out << indent << "\"class_objects\": " << summary.classObjects << ",\n";
    out << indent << "\"script_struct_objects\": " << summary.scriptStructObjects << ",\n";
    out << indent << "\"state_objects\": " << summary.stateObjects << ",\n";
    out << indent << "\"property_objects\": " << summary.propertyObjects << ",\n";
    out << indent << "\"function_objects\": " << summary.functionObjects << ",\n";
    out << indent << "\"enum_objects\": " << summary.enumObjects << ",\n";
    out << indent << "\"default_objects\": " << summary.defaultObjects << ",\n";
    out << indent << "\"objects_with_bytecode\": " << summary.objectsWithBytecode << ",\n";
    out << indent << "\"known_packages_present\": ";
    WriteStringArray(out, summary.knownPackagesPresent);
    out << ",\n";
    out << indent << "\"known_packages_missing\": ";
    WriteStringArray(out, summary.knownPackagesMissing);
    out << ",\n";
    out << indent << "\"package_counts\": ";
    WriteStringIntMap(out, summary.packageCounts);
    out << ",\n";
    out << indent << "\"class_counts\": ";
    WriteStringIntMap(out, summary.classCounts);
    out << ",\n";
    out << indent << "\"property_kind_counts\": ";
    WriteStringIntMap(out, summary.propertyKindCounts);
    out << "\n";
}

bool WriteAuditSummaryJson(const Kuru::Common::RocketLeagueGlobals& globals, const DumpSummary& summary) {
    std::ofstream out(OutputPath("audit_summary.json"), std::ios::trunc);
    if (!out.is_open()) {
        Log("Failed to open audit_summary.json for writing");
        return false;
    }

    out << "{\n";
    out << "  \"version\": " << Kuru::Common::DumpSchema::CurrentVersion << ",\n";
    out << "  \"created_utc\": \"" << Json(TimestampUtc()) << "\",\n";
    out << "  \"game\": \"Rocket League\",\n";
    out << "  \"exe_path\": \"" << Json(ExePath()) << "\",\n";
    out << "  \"exe_version\": \"" << Json(ExeFileVersion()) << "\",\n";
    out << "  \"ue3_version\": \"" << Json(UE3VersionString()) << "\",\n";
    out << "  \"build_label\": \"" << Json(BuildLabel()) << "\",\n";
    out << "  \"psyonix_build_id\": " << PsyonixBuildId(globals) << ",\n";
    out << "  \"psyonix_build_id_string\": \"" << Json(PsyonixBuildIdString(globals)) << "\",\n";
    out << "  \"globals\": {\n";
    out << "    \"GNames\": \"" << Hex(globals.GNamesAddress) << "\",\n";
    out << "    \"GObjects\": \"" << Hex(globals.GObjectsAddress) << "\",\n";
    out << "    \"GMalloc\": \"" << Hex(globals.GMallocAddress) << "\",\n";
    out << "    \"GMallocValue\": \"" << Hex(globals.GMallocValue) << "\",\n";
    out << "    \"GPsyonixBuildID\": \"" << Hex(globals.PsyonixBuildIDAddress) << "\",\n";
    out << "    \"BuildDate\": \"" << Hex(globals.BuildDateAddress) << "\"\n";
    out << "  },\n";
    out << "  \"build_date\": \"" << Json(globals.BuildDate.empty() ? "unknown" : globals.BuildDate) << "\",\n";
    out << "  \"raw_counts\": {\n";
    out << "    \"names\": " << globals.NameCount << ",\n";
    out << "    \"objects\": " << globals.ObjectCount << "\n";
    out << "  },\n";
    out << "  \"summary\": {\n";
    WriteSummaryJson(out, summary, "    ");
    out << "  }\n";
    out << "}\n";
    Log("Wrote audit_summary.json");
    return true;
}

void WriteNamesArray(std::ostream& out, const Kuru::Common::RocketLeagueGlobals& globals) {
    out << "  \"names\": [";
    for (int i = 0; i < globals.NameCount; ++i) {
        if (i != 0) {
            out << ",";
        }
        out << "\"";
        auto value = Kuru::Common::TryReadFNameEntryString(globals.GNames, FName{i, 0});
        if (value.has_value()) {
            out << Json(*value);
        }
        out << "\"";
    }
    out << "],\n";
}

bool WriteGlobalsJson(const Kuru::Common::RocketLeagueGlobals& globals) {
    std::ofstream out(OutputPath("globals.json"), std::ios::trunc);
    if (!out.is_open()) {
        Log("Failed to open globals.json for writing");
        return false;
    }

    out << "{\n";
    out << "  \"version\": " << Kuru::Common::DumpSchema::CurrentVersion << ",\n";
    out << "  \"created_utc\": \"" << Json(TimestampUtc()) << "\",\n";
    out << "  \"exe_path\": \"" << Json(ExePath()) << "\",\n";
    out << "  \"exe_version\": \"" << Json(ExeFileVersion()) << "\",\n";
    out << "  \"ue3_version\": \"" << Json(UE3VersionString()) << "\",\n";
    out << "  \"build_label\": \"" << Json(BuildLabel()) << "\",\n";
    out << "  \"psyonix_build_id\": " << PsyonixBuildId(globals) << ",\n";
    out << "  \"psyonix_build_id_string\": \"" << Json(PsyonixBuildIdString(globals)) << "\",\n";
    out << "  \"patterns\": {\n";
    out << "    \"GNames\": \"" << Json(GNamesPattern) << "\",\n";
    out << "    \"GMalloc\": \"" << Json(GMallocPattern) << "\",\n";
    out << "    \"GPsyonixBuildID\": \"" << Json(GPsyonixBuildIDPattern) << "\",\n";
    out << "    \"BuildDate\": \"" << Json(BuildDatePattern) << "\"\n";
    out << "  },\n";
    out << "  \"globals\": {\n";
    out << "    \"GNames\": \"" << Hex(globals.GNamesAddress) << "\",\n";
    out << "    \"GObjects\": \"" << Hex(globals.GObjectsAddress) << "\",\n";
    out << "    \"GMalloc\": \"" << Hex(globals.GMallocAddress) << "\",\n";
    out << "    \"GMallocValue\": \"" << Hex(globals.GMallocValue) << "\",\n";
    out << "    \"GPsyonixBuildID\": \"" << Hex(globals.PsyonixBuildIDAddress) << "\",\n";
    out << "    \"BuildDate\": \"" << Hex(globals.BuildDateAddress) << "\"\n";
    out << "  },\n";
    out << "  \"build_date\": \"" << Json(globals.BuildDate.empty() ? "unknown" : globals.BuildDate) << "\",\n";
    out << "  \"counts\": {\n";
    out << "    \"names\": " << globals.NameCount << ",\n";
    out << "    \"objects\": " << globals.ObjectCount << "\n";
    out << "  }\n";
    out << "}\n";
    Log("Wrote globals.json");
    return true;
}

bool WriteDumpJson(const Kuru::Common::RocketLeagueGlobals& globals) {
    std::ofstream out(OutputPath("dump.json"), std::ios::trunc);
    if (!out.is_open()) {
        Log("Failed to open dump.json for writing");
        return false;
    }

    out << "{\n";
    out << "  \"version\": " << Kuru::Common::DumpSchema::CurrentVersion << ",\n";
    out << "  \"game\": \"Rocket League\",\n";
    out << "  \"created_utc\": \"" << Json(TimestampUtc()) << "\",\n";
    out << "  \"exe_path\": \"" << Json(ExePath()) << "\",\n";
    out << "  \"exe_version\": \"" << Json(ExeFileVersion()) << "\",\n";
    out << "  \"ue3_version\": \"" << Json(UE3VersionString()) << "\",\n";
    out << "  \"build_label\": \"" << Json(BuildLabel()) << "\",\n";
    out << "  \"psyonix_build_id\": " << PsyonixBuildId(globals) << ",\n";
    out << "  \"psyonix_build_id_string\": \"" << Json(PsyonixBuildIdString(globals)) << "\",\n";
    out << "  \"build_date\": \"" << Json(globals.BuildDate.empty() ? "unknown" : globals.BuildDate) << "\",\n";
    out << "  \"globals\": {\n";
    out << "    \"GNames\": \"" << Hex(globals.GNamesAddress) << "\",\n";
    out << "    \"GObjects\": \"" << Hex(globals.GObjectsAddress) << "\",\n";
    out << "    \"GMalloc\": \"" << Hex(globals.GMallocAddress) << "\",\n";
    out << "    \"GMallocValue\": \"" << Hex(globals.GMallocValue) << "\",\n";
    out << "    \"GPsyonixBuildID\": \"" << Hex(globals.PsyonixBuildIDAddress) << "\",\n";
    out << "    \"BuildDate\": \"" << Hex(globals.BuildDateAddress) << "\"\n";
    out << "  },\n";
    out << "  \"name_count\": " << globals.NameCount << ",\n";
    out << "  \"object_count\": " << globals.ObjectCount << ",\n";
    WriteNamesArray(out, globals);
    out << "  \"objects\": [\n";

    bool first = true;
    DumpSummary summary{};

    for (int index = 0; index < globals.ObjectCount; ++index) {
        if (index != 0 && (index % 1000) == 0) {
            Log("Dump progress index=" + std::to_string(index) + "/" + std::to_string(globals.ObjectCount) +
                " written=" + std::to_string(summary.writtenObjects));
        }

        const auto* object = globals.GObjects->Data[index];
        if (!Kuru::Common::LooksLikeUObject(object, globals.GNames)) {
            continue;
        }

        const std::string className = ClassName(globals, object);
        const std::string objectName = ObjectName(globals, object);
        const std::string fullName = ObjectFullName(globals, object);
        const std::string packageName = PackageNameFromFullName(fullName);
        const bool isStructLike = IsStructLikeClassName(className);
        const bool isProperty = IsPropertyClassName(className);
        const bool isFunction = className == "Function";

        ++summary.classCounts[className];
        ++summary.packageCounts[packageName];
        if (isStructLike) {
            ++summary.structLikeObjects;
        }
        if (className == "Class") {
            ++summary.classObjects;
        }
        if (className == "ScriptStruct") {
            ++summary.scriptStructObjects;
        }
        if (className == "State") {
            ++summary.stateObjects;
        }
        if (className == "Enum") {
            ++summary.enumObjects;
        }
        if (isProperty) {
            ++summary.propertyObjects;
            ++summary.propertyKindCounts[className];
        }
        if (isFunction) {
            ++summary.functionObjects;
        }
        if (StartsWith(objectName, "Default__")) {
            ++summary.defaultObjects;
        }

        if (!first) {
            out << ",\n";
        }
        first = false;

        out << "    {";
        out << "\"index\":" << index << ",";
        out << "\"address\":\"" << Hex(reinterpret_cast<std::uintptr_t>(object)) << "\",";
        out << "\"object_flags\":\"" << Hex(object->ObjectFlags) << "\",";
        out << "\"name\":\"" << Json(objectName) << "\",";
        out << "\"full_name\":\"" << Json(fullName) << "\",";
        out << "\"package_name\":\"" << Json(packageName) << "\",";
        out << "\"class_name\":\"" << Json(className) << "\",";
        out << "\"outer_name\":\"" << Json(object->Outer ? ObjectFullName(globals, object->Outer) : "None") << "\"";
        WriteCoreLayoutProbe(out, globals, object, fullName, className);

        if (isStructLike) {
            const auto* strct = reinterpret_cast<const UStructRuntime*>(object);
            if (ReadableObject(strct)) {
                if (strct->Script.Count > 0) {
                    ++summary.objectsWithBytecode;
                }
                out << ",\"super_name\":\"" << Json(strct->SuperStruct ? ObjectFullName(globals, strct->SuperStruct) : "None") << "\"";
                out << ",\"children\":\"" << Hex(reinterpret_cast<std::uintptr_t>(strct->Children)) << "\"";
                out << ",\"property_size\":" << strct->PropertiesSize;
                out << ",\"min_alignment\":" << strct->MinAlignment;
                out << ",\"bytecode_len\":" << strct->Script.Count;
                out << ",\"properties\":";
                WritePropertiesArray(out, globals, strct);
                out << ",\"functions\":";
                WriteFunctionsArray(out, globals, strct);

                if (className == "Class") {
                    WriteClassMetadata(out, globals, reinterpret_cast<const UClassRuntime*>(object));
                }
            }
        }

        if (className == "Enum") {
            WriteEnumMetadata(out, globals, reinterpret_cast<const UEnumRuntime*>(object));
        }

        if (StartsWith(objectName, "Default__")) {
            out << ",\"default_properties\":";
            WriteDefaultProperties(out, globals, object);
        }

        if (isProperty) {
            out << ",\"property\":";
            WritePropertyJson(out, globals, reinterpret_cast<const UPropertyRuntime*>(object));
        }

        if (isFunction) {
            const auto* fn = reinterpret_cast<const UFunctionRuntime*>(object);
            if (ReadableObject(fn)) {
                out << ",\"function_flags\":\"" << Hex(fn->FunctionFlags) << "\"";
                out << ",\"parms_size\":" << fn->ParmsSize;
                out << ",\"num_parms\":" << static_cast<int>(fn->NumParms);
                out << ",\"return_value_offset\":" << fn->ReturnValueOffset;
                out << ",\"func\":\"" << Hex(reinterpret_cast<std::uintptr_t>(fn->Func)) << "\"";
            }
        }

        out << "}";
        ++summary.writtenObjects;
    }

    FillKnownPackagePresence(summary);

    out << "\n  ],\n";
    out << "  \"written_object_count\": " << summary.writtenObjects << ",\n";
    out << "  \"summary\": {\n";
    WriteSummaryJson(out, summary, "    ");
    out << "  }\n";
    out << "}\n";

    const bool wroteAuditSummary = WriteAuditSummaryJson(globals, summary);

    Log("Wrote dump.json object inventory count=" + std::to_string(summary.writtenObjects) +
        " structs=" + std::to_string(summary.structLikeObjects) +
        " properties=" + std::to_string(summary.propertyObjects) +
        " functions=" + std::to_string(summary.functionObjects));

    return wroteAuditSummary;
}

void LogSampleObjects(const Kuru::Common::RocketLeagueGlobals& globals) {
    if (!globals.GObjects || globals.ObjectCount <= 0) {
        return;
    }

    Log("Sample objects:");
    const int stride = globals.ObjectCount > 8 ? globals.ObjectCount / 8 : 1;
    int printed = 0;

    for (int index = 0; index < globals.ObjectCount && printed < 8; index += stride) {
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

DWORD WINAPI MainThread(LPVOID parameter) {
    const auto selfModule = reinterpret_cast<HMODULE>(parameter);
    if (!InitializeOutputDirectory(selfModule)) {
        gOutputDirectory = ".";
        NotifyUser("KuruDump failed",
                   "Could not resolve the KuruDump DLL path, so the dumper cannot choose an output directory.",
                   MB_ICONERROR);
        return 0;
    }

    OpenLog();
    Log("Output directory: " + gOutputDirectory);
    Log("Injected; scanning for GNames using known-good Kuru-dump pattern");

    const auto match = Kuru::Common::FindPatternInCurrentModule(GNamesPattern);
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
    globals.GNamesAddress = reinterpret_cast<std::uintptr_t>(validation.gNames);
    globals.GObjectsAddress = reinterpret_cast<std::uintptr_t>(validation.gObjects);
    globals.NameCount = validation.gNames->Count;
    globals.ObjectCount = validation.gObjects->Count;

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

    Log("GNames validated at " + Hex(globals.GNamesAddress) + " count=" + std::to_string(globals.NameCount));
    Log("GObjects validated at " + Hex(globals.GObjectsAddress) + " count=" + std::to_string(globals.ObjectCount));
    Log("Valid UObject samples=" + std::to_string(validation.validObjectSamples));

    LogSampleObjects(globals);
    if (!WriteGlobalsJson(globals)) {
        return FailMainThread("Failed to write globals.json");
    }
    if (!WriteDumpJson(globals)) {
        return FailMainThread("Failed to write dump.json or audit_summary.json");
    }
    Log("Reflection dump complete");
    NotifyUser("KuruDump complete",
               "Reflection dump complete.\n\nFiles written to:\n" + gOutputDirectory +
                   "\n\n- dump.json\n- globals.json\n- audit_summary.json\n- KuruDump.log",
               MB_ICONINFORMATION);

    return 0;
}

} // namespace

BOOL APIENTRY DllMain(HMODULE module, DWORD reason, LPVOID reserved) {
    (void)reserved;

    switch (reason) {
    case DLL_PROCESS_ATTACH:
        DisableThreadLibraryCalls(module);
        CreateThread(nullptr, 0, MainThread, module, 0, nullptr);
        break;
    case DLL_PROCESS_DETACH:
        Kuru::Common::ClearRocketLeagueGlobals();
        if (gLog.is_open()) {
            gLog.close();
        }
        break;
    default:
        break;
    }

    return TRUE;
}
