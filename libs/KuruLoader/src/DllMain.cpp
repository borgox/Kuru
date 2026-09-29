/* Kuru mod loader host.
1. Resolve the RL globals
2. Load mods
3. After all mods have registered their hooks, enable the detours
*/
#include "Console.hpp"
#include "Gui.hpp"
#include "HookEngine.hpp"
#include "ModRegistry.hpp"
#include "Variables.hpp"

#include <Core/Flags.hpp>
#include <Core/Globals.hpp>
#include <Core/ObjectModel.hpp>
#include <Core/Runtime.hpp>
#include <Kuru/HostApi.hpp>
#include <Kuru/Mod.hpp>

#include <Kuru/Common/Globals.hpp>
#include <Kuru/Common/Unreal.hpp>

#include <windows.h>

#include <algorithm>
#include <cstdint>
#include <cstdio>
#include <cstring>
#include <filesystem>
#include <ranges>
#include <string>
#include <vector>

namespace {

HMODULE gSelfModule = nullptr;
void* gProcessEventAddress = nullptr; // slot 67
void* gProcessDelegateAddress = nullptr; // slot 68
void* gRemoteFunctionAddress = nullptr;  // slot 71
void* gCallDelegateAddress = nullptr;        // slot 69
void** gVTable = nullptr;                // a live UObject vtable
constexpr int kCallFunctionVTableIndex = 76;
// Mapping done, flip back to true to find the vtable region if a game update changes it.
constexpr bool kMappingMode = false;


void* gPIExact = nullptr;  // resolved ProcessInternal address

// debugger output (DebugView) and the in-game console
void Log(const std::string& message) {
    OutputDebugStringA(("[Kuru] " + message + "\n").c_str());
    Kuru::Console::AddLog(message.c_str());
}

// [base,size) of the injected game's main module from its PE header.
bool MainModuleRange(std::uintptr_t& base, std::size_t& size) {
    HMODULE module = GetModuleHandleW(nullptr);
    if (!module) {
        return false;
    }
    base = reinterpret_cast<std::uintptr_t>(module);
    const auto* dos = reinterpret_cast<IMAGE_DOS_HEADER*>(base);
    const auto* nt = reinterpret_cast<IMAGE_NT_HEADERS*>(base + dos->e_lfanew);
    size = nt->OptionalHeader.SizeOfImage;
    return true;
}

// [start,end) of the main module's executable section, used to validate that GNatives entries are code.
bool TextSectionRange(std::uintptr_t& start, std::uintptr_t& end) {
    HMODULE module = GetModuleHandleW(nullptr);
    if (!module) {
        return false;
    }
    const auto base = reinterpret_cast<std::uintptr_t>(module);
    const auto* dos = reinterpret_cast<IMAGE_DOS_HEADER*>(base);
    const auto* nt = reinterpret_cast<IMAGE_NT_HEADERS*>(base + dos->e_lfanew);
    const auto* section = IMAGE_FIRST_SECTION(nt);
    for (unsigned i = 0; i < nt->FileHeader.NumberOfSections; ++i) {
        if (section[i].Characteristics & IMAGE_SCN_MEM_EXECUTE) {
            start = base + section[i].VirtualAddress;
            end = start + section[i].Misc.VirtualSize;
            return true;
        }
    }
    return false;
}


// SEH-guarded check: verify that `addr` points to 256 pointers, all within the text section (GNatives opcode -> handler table).
bool LooksLikeGNatives(std::uintptr_t addr, std::uintptr_t textStart, std::uintptr_t textEnd) {
    __try {
        const auto* table = reinterpret_cast<const std::uintptr_t*>(addr);
        for (int i = 0; i < 256; ++i) {
            const std::uintptr_t handler = table[i];
            if (handler < textStart || handler >= textEnd) {
                return false;
            }
        }
        return true;
    } __except (EXCEPTION_EXECUTE_HANDLER) {
        return false;
    }
}

/* Resolve GNatives (the bytecode opcode->handler table) WITHOUT a hardcoded RVA. It's referenced inside
 * ProcessInternal. We scan PI's code
 * for the table access; either a direct `lea reg,[rip+disp]` to the table or the form this build
 * uses (`lea reg,[rip+X]` + `mov reg,[reg+idx*8+disp]`) and validate each candidate is 256 code pointers.
 */
void* ResolveGNatives(void* processInternal) {
    if (!processInternal) {
        return nullptr;
    }
    std::uintptr_t textStart = 0;
    std::uintptr_t textEnd = 0;
    if (!TextSectionRange(textStart, textEnd)) {
        return nullptr;
    }
    const auto* code = static_cast<const std::uint8_t*>(processInternal);
    constexpr std::size_t kScan = 0x400;
    std::vector<std::uintptr_t> leaTargets;  // RIP-relative lea destinations (candidate table bases / anchors)
    std::vector<std::int32_t> sibDisps;       // disp32 of [base + index*8 + disp32] accesses
    for (std::size_t i = 0; i + 8 <= kScan; ++i) {
        // lea r64, [rip+disp32] : REX.W(0x48..0x4F) 8D ModRM(mod=00, rm=101)
        if ((code[i] & 0xF8) == 0x48 && code[i + 1] == 0x8D && (code[i + 2] & 0xC7) == 0x05) {
            const auto disp = *reinterpret_cast<const std::int32_t*>(code + i + 3);
            leaTargets.push_back(reinterpret_cast<std::uintptr_t>(code + i + 7) + disp);
        }
        // mov r64, [base + idx*8 + disp32] : REX(0x40..0x4F) 8B ModRM(mod=10, rm=100) SIB(scale=8)
        if ((code[i] & 0xF0) == 0x40 && code[i + 1] == 0x8B && (code[i + 2] & 0xC7) == 0x84 &&
            (code[i + 3] & 0xC0) == 0xC0) {
            sibDisps.push_back(*reinterpret_cast<const std::int32_t*>(code + i + 4));
        }
    }
    for (const std::uintptr_t target : leaTargets) {
        if (LooksLikeGNatives(target, textStart, textEnd)) {
            return reinterpret_cast<void*>(target);  // direct lea to the table
        }
        for (const std::int32_t disp : sibDisps) {   // candidate table = lea anchor + disp32
            const auto candidate = target + static_cast<std::uintptr_t>(static_cast<std::intptr_t>(disp));
            if (LooksLikeGNatives(candidate, textStart, textEnd)) {
                return reinterpret_cast<void*>(candidate);
            }
        }
    }
    return nullptr;
}

// Collect RIP-relative qword store targets (`mov [rip+disp32], r64`) in a handler's prologue.
int FindGlobalStoreTargets(const std::uint8_t* code, std::size_t scan, std::uintptr_t* out, int maxOut) {
    int n = 0;
    for (std::size_t i = 0; i + 7 <= scan && n < maxOut; ++i) {
        if ((code[i] & 0xF8) == 0x48 && code[i + 1] == 0x89 && (code[i + 2] & 0xC7) == 0x05) {
            const auto disp = *reinterpret_cast<const std::int32_t*>(code + i + 3);
            out[n++] = reinterpret_cast<std::uintptr_t>(code + i + 7) + disp;
        }
    }
    return n;
}

/* Call a candidate variable-access handler with a fake frame, Result=NULL so it only SETS its globals
 * (GProperty=prop, GPropAddr=obj+Offset, GPropObject=obj/NULL) and copies nothing, then match the store
 * targets by value to identify which is which. SEH-guarded. These globals are dead between script Steps
 * so writing them from off-thread is harmless.
*/
bool ProbeInterpreterGlobals(void* handler, Kuru::SDK::UObject* probeObj, Kuru::SDK::UProperty* probeProp,
                             const std::uintptr_t* targets, int nTargets, std::uintptr_t lo, std::uintptr_t hi,
                             void*& outProp, void*& outAddr, void*& outObj) {
    alignas(16) std::uint8_t codeBuf[64] = {};
    *reinterpret_cast<void**>(codeBuf) = probeProp;         // Stack.Code -> the property pointer (ReadObject)
    alignas(16) std::uint8_t frameBuf[0x60] = {};
    *reinterpret_cast<void**>(frameBuf + 0x20) = probeObj;  // FFrame.Object
    *reinterpret_cast<void**>(frameBuf + 0x28) = codeBuf;   // FFrame.Code
    *reinterpret_cast<void**>(frameBuf + 0x30) = probeObj;  // FFrame.Locals (so a local-var handler also gives obj+Offset)
    const auto expectAddr =
        reinterpret_cast<std::uintptr_t>(probeObj) + static_cast<std::uintptr_t>(probeProp->Offset);
    using HandlerFn = void (*)(void*, void*, void*);
    __try {
        reinterpret_cast<HandlerFn>(handler)(probeObj, frameBuf, nullptr);
    } __except (EXCEPTION_EXECUTE_HANDLER) {
        return false;
    }
    void* prop = nullptr;
    void* addr = nullptr;
    void* obj = nullptr;
    for (int i = 0; i < nTargets; ++i) {
        if (targets[i] < lo || targets[i] >= hi) {
            continue;  // not a data global
        }
        const auto val = *reinterpret_cast<std::uintptr_t*>(targets[i]);
        if (val == reinterpret_cast<std::uintptr_t>(probeProp)) {
            prop = reinterpret_cast<void*>(targets[i]);
        } else if (val == expectAddr) {
            addr = reinterpret_cast<void*>(targets[i]);
        } else {
            obj = reinterpret_cast<void*>(targets[i]);
        }
    }
    if (prop && addr) {
        outProp = prop;
        outAddr = addr;
        outObj = obj;
        return true;
    }
    return false;
}

/* Resolve the interpreter runtime globals (GProperty/GPropAddr/GPropObject/GRuntimeUCFlags) WITHOUT
 * hardcoded RVAs, so they survive game updates. A variable-access handler in GNatives writes the first
 * three as RIP-relative stores in its prologue, and we find that handler (>=3 such stores), extract the
 * targets, and probe it with a known property+object to disambiguate. GRuntimeUCFlags is the
 * `and dword [rip], ~RUC` store in CallFunction. Returns true if at least GPropAddr resolved.
 */
bool ResolveInterpreterGlobals(void* gnatives, void* callFunctionAddr, void*& outProp, void*& outAddr,
                               void*& outObj, void*& outFlags) {
    outProp = outAddr = outObj = outFlags = nullptr;
    std::uintptr_t base = 0;
    std::size_t size = 0;
    if (!gnatives || !MainModuleRange(base, size)) {
        return false;
    }
    const std::uintptr_t lo = base;
    const std::uintptr_t hi = base + size;

    // A probe property + a valid object (the property's address in it is only COMPUTED, never read).
    Kuru::SDK::UProperty* probeProp = nullptr;
    auto* probeObj = reinterpret_cast<Kuru::SDK::UObject*>(Kuru::SDK::UObject::StaticClass());
    if (Kuru::SDK::UClass* propClass = Kuru::SDK::UProperty::StaticClass()) {
        if (auto* objects = Kuru::SDK::UObject::GetGlobalObjects()) {
            for (Kuru::SDK::UObject* o : *objects) {
                if (o && o->IsA(propClass)) {
                    auto* p = reinterpret_cast<Kuru::SDK::UProperty*>(o);
                    if (p->Offset > 0) {  // non-zero so GPropAddr (obj+Offset) != GPropObject (obj), no collision
                        probeProp = p;
                        break;
                    }
                }
            }
        }
    }
    if (!probeProp || !probeObj) {
        return false;
    }

    auto* table = reinterpret_cast<void**>(gnatives);
    for (int i = 0; i < 256; ++i) {
        void* handler = table[i];
        const auto h = reinterpret_cast<std::uintptr_t>(handler);
        if (h < lo || h >= hi) {
            continue;
        }
        std::uintptr_t targets[8];
        const int nt = FindGlobalStoreTargets(reinterpret_cast<const std::uint8_t*>(handler), 0x40, targets, 8);
        if (nt >= 3 &&
            ProbeInterpreterGlobals(handler, probeObj, probeProp, targets, nt, lo, hi, outProp, outAddr, outObj)) {
            break;
        }
    }

    // GRuntimeUCFlags: `and dword ptr [rip+disp32], 0FFFFFFFDh` == 83 25 disp32 FD (AND is /4 -> ModRM 0x25).
    if (callFunctionAddr) {
        const auto* c = reinterpret_cast<const std::uint8_t*>(callFunctionAddr);
        for (std::size_t i = 0; i + 7 <= 0x2000; ++i) {
            if (c[i] == 0x83 && c[i + 1] == 0x25 && c[i + 6] == 0xFD) {
                const auto disp = *reinterpret_cast<const std::int32_t*>(c + i + 2);
                const auto target = reinterpret_cast<std::uintptr_t>(c + i + 7) + disp;
                if (target >= lo && target < hi) {
                    outFlags = reinterpret_cast<void*>(target);
                    break;
                }
            }
        }
    }
    return outAddr != nullptr;
}

/* ProcessInternal is non-virtual, to resolve it, we check every UFunction::Func, if that function is native,
 * it's the native impl, for a script function (non-native) its &UObject::ProcessInternal, so PI is the Func value stored
 * by the script function
*/
void ResolveProcessInternal() {
    Kuru::SDK::TArray<Kuru::SDK::UObject*>* objects = Kuru::SDK::UObject::GetGlobalObjects();
    const Kuru::SDK::UClass* funcClass = Kuru::SDK::UFunction::StaticClass(); // the UClass representing UFunction
    if (!objects || !funcClass) {
        Log("ProcessInternal: GObjects/UFunction class unavailable");
        return;
    }
    for (Kuru::SDK::UObject* obj : *objects) {
        if (!obj || !obj->IsA(funcClass)) {
            continue;
        }
        const auto* fn = reinterpret_cast<Kuru::SDK::UFunction*>(obj);
        if ((fn->FunctionFlags & Kuru::SDK::FUNC_Native) != 0) {
            continue;  // native: Func is the C++ thunk, not ProcessInternal
        }
        void* pi = reinterpret_cast<void*>(fn->Func);
        if (!pi) {
            continue;
        }
        gPIExact = pi;
        std::size_t size = 0;
        char buf[128];
        if (std::uintptr_t base = 0; MainModuleRange(base, size)) {
            std::snprintf(buf, sizeof(buf), "0x%p  RVA=0x%llx  (via '%s')", pi,
                          static_cast<unsigned long long>(reinterpret_cast<std::uintptr_t>(pi) - base),
                          obj->GetName().c_str());
        } else {
            std::snprintf(buf, sizeof(buf), "0x%p  (via '%s')", pi, obj->GetName().c_str());
        }
        Log(std::string("ProcessInternal resolved via UFunction::Func: ") + buf);
        return;
    }
    Log("ProcessInternal not resolved (no non-native UFunction found)");
}

std::filesystem::path SelfDirectory() {
    wchar_t buffer[MAX_PATH]{};
    const DWORD length = GetModuleFileNameW(gSelfModule, buffer, MAX_PATH);
    if (length == 0 || length == MAX_PATH) {
        return {};
    }
    return std::filesystem::path(buffer).parent_path();
}

// Resolve RL globals and publish them to the SDK. Returns false if the game memory could not be validated
bool BootstrapGlobals() {
    // GNames pattern
    constexpr std::string_view GNamesPattern = "?? ?? ?? ?? ?? ?? 00 00 ?? ?? ?? ?? 35 25 02 00";

    const auto globals = Kuru::Common::TryInitializeRocketLeagueGlobalsFromPattern(GNamesPattern);
    if (!globals.has_value() || globals->GNames == nullptr || globals->GObjects == nullptr) {
        Log("global bootstrap failed: GNames/GObjects not resolved");
        return false;
    }

    Kuru::SDK::RuntimeBindings bindings{};
    bindings.Names = reinterpret_cast<Kuru::SDK::TArray<Kuru::SDK::FNameEntry*>*>(
        const_cast<Kuru::Common::TArray<void*>*>(globals->GNames));
    bindings.Objects = reinterpret_cast<Kuru::SDK::TArray<Kuru::SDK::UObject*>*>(
        const_cast<Kuru::Common::TArray<Kuru::Common::UObjectPrefix*>*>(globals->GObjects));

    /*
     * Resolve ProcessEvent from any live UObject's vtable, every UObject shares this slot, so the first valid
     * object is works, this is the address used both to CALL functions (ProcessEventChecked) and to install the hook detour.
    */
    if (globals->GObjects->Count > 0 && globals->GObjects->Data) {
        for (std::int32_t i = 0; i < globals->GObjects->Count; ++i) {
            if (Kuru::Common::UObjectPrefix* obj = globals->GObjects->Data[i]; obj && obj->VfTableObject) {
                constexpr int kProcessEventVTableIndex = 67;
                const auto vtable = static_cast<void**>(obj->VfTableObject);
                gVTable = vtable;
                void* peAddr = vtable[kProcessEventVTableIndex];
                gProcessEventAddress = peAddr;
                bindings.ProcessEvent = reinterpret_cast<Kuru::SDK::ProcessEventFunc>(peAddr);
                char buf[32];
                std::snprintf(buf, sizeof(buf), "0x%p", peAddr);
                Log(std::string("ProcessEvent resolved via vtable[67] = ") + buf);
                constexpr int kProcessDelegateVTableIndex = 68;
                gProcessDelegateAddress = vtable[kProcessDelegateVTableIndex];
                std::snprintf(buf, sizeof(buf), "0x%p", gProcessDelegateAddress);
                Log(std::string("ProcessDelegate vtable[68] = ") + buf);
                constexpr int kRemoteFunctionVTableIndex = 71;
                gRemoteFunctionAddress = vtable[kRemoteFunctionVTableIndex];
                std::snprintf(buf, sizeof(buf), "0x%p", gRemoteFunctionAddress);
                Log(std::string("ProcessRemoteFunction vtable[71] = ") + buf);
                constexpr int kCallDelegateVTableIndex = 69;
                gCallDelegateAddress = vtable[kCallDelegateVTableIndex];
                std::snprintf(buf, sizeof(buf), "0x%p", gCallDelegateAddress);
                Log(std::string("CallDelegate vtable[69] = ") + buf);
                // missing CF??
                (void)peAddr;  // ProcessInternal resolves from UFunction::Func after bindings are set
                break;
            }
        }
    }
    if (!bindings.ProcessEvent) {
        Log("WARNING: ProcessEvent not resolved (no valid object vtable), calls/hooks disabled");
    }

    // GMalloc + PI/PD/PRF/etc... bindings are resolved as their features land.
    Kuru::SDK::SetRuntimeBindings(bindings);

    Log("globals bootstrapped: names=" + std::to_string(globals->NameCount) +
        " objects=" + std::to_string(globals->ObjectCount));

    // Now that GObjects is live, resolve ProcessInternal from UFunction::Func
    ResolveProcessInternal();

    // Resolve GNatives (the bytecode interpreter's opcode->handler table) out of ProcessInternal, so the
    // CallFunction hook can read native/body-less call args via the game's own interpreter
    void* gnatives = ResolveGNatives(gPIExact);
    if (gnatives) {
        Kuru::HookEngine::SetGNatives(gnatives);
        char buf[64];
        std::snprintf(buf, sizeof(buf), "0x%p", gnatives);
        Log(std::string("GNatives resolved (interpreter table) at ") + buf);
    } else {
        Log("GNatives NOT resolved, CallFunction arg reading unavailable");
    }

    // Interpreter runtime globals (GProperty/GPropAddr/GPropObject/GRuntimeUCFlags), needed for CF param
    // editing. Resolved at runtime from the handler code,  the RVAs are only a fallback.
    {
        void* gProp = nullptr;
        void* gAddr = nullptr;
        void* gObj = nullptr;
        void* gFlags = nullptr;
        void* cfAddr = gVTable ? gVTable[kCallFunctionVTableIndex] : nullptr;
        if (gnatives && ResolveInterpreterGlobals(gnatives, cfAddr, gProp, gAddr, gObj, gFlags)) {
            Kuru::HookEngine::SetInterpreterGlobals(gProp, gAddr, gObj, gFlags);
            char buf[160];
            std::snprintf(buf, sizeof(buf), "GPropAddr=0x%p GProperty=0x%p GPropObject=0x%p GRuntimeUCFlags=0x%p",
                          gAddr, gProp, gObj, gFlags);
            Log(std::string("interpreter globals resolved at runtime: ") + buf);
        } else if (HMODULE module = GetModuleHandleW(nullptr)) {
            const auto b = reinterpret_cast<std::uintptr_t>(module);  // build-specific RVAs,last resort
            Kuru::HookEngine::SetInterpreterGlobals(
                reinterpret_cast<void*>(b + 0x22D6840), reinterpret_cast<void*>(b + 0x22D6870),
                reinterpret_cast<void*>(b + 0x22D6878), reinterpret_cast<void*>(b + 0x22D67B4));
            Log("interpreter globals: runtime resolve FAILED, using RVA fallback)");
        }
    }
    return true;
}

void LoadModsFrom(const std::filesystem::path& modsDir) {
    std::error_code ec;
    if (!std::filesystem::is_directory(modsDir, ec)) {
        Log("no mods directory at " + modsDir.string());
        return;
    }

    Log("scanning mods directory: " + modsDir.string());
    int dllCount = 0;
    for (const auto& entry : std::filesystem::directory_iterator(modsDir, ec)) {
        if (entry.is_regular_file() && entry.path().extension() == L".dll") {
            ++dllCount;
        }
    }
    Log("found " + std::to_string(dllCount) + " .dll file(s) in mods directory");

    for (const auto& entry : std::filesystem::directory_iterator(modsDir, ec)) {
        if (!entry.is_regular_file() || entry.path().extension() != L".dll") {
            continue;
        }

        const std::wstring dllPath = entry.path().wstring();
        const HMODULE dll = LoadLibraryW(dllPath.c_str());
        if (!dll) {
            Log("LoadLibrary failed for " + entry.path().string());
            continue;
        }

        using GetModInfoFunc = const Kuru::ModInfo* (*)();
        const auto getInfo = reinterpret_cast<GetModInfoFunc>(GetProcAddress(dll, "KuruGetModInfo"));
        const auto create = reinterpret_cast<Kuru::CreateModFunc>(GetProcAddress(dll, "KuruCreateMod"));
        const auto destroy = reinterpret_cast<Kuru::DestroyModFunc>(GetProcAddress(dll, "KuruDestroyMod"));
        if (!getInfo || !create || !destroy) {
            Log("not a Kuru mod (missing exports): " + entry.path().string());
            FreeLibrary(dll);
            continue;
        }

        // Version gate: skip a mod built against a NEWER Kuru than this loader provides
        using ApiVersionFunc = std::uint32_t (*)();
        if (const auto reqVersion = reinterpret_cast<ApiVersionFunc>(GetProcAddress(dll, "KuruRequiredApiVersion"))) {
            if (const std::uint32_t need = reqVersion(); need > Kuru::HostApiVersion) {
                Log("skipping mod built for a newer Kuru (needs API v" + std::to_string(need) + ", loader is v" +
                    std::to_string(Kuru::HostApiVersion) + "): " + entry.path().string());
                FreeLibrary(dll);
                continue;
            }
        }

        // Push the resolved game bindings into this mod's own SDK copy before it runs
        using InitRuntimeFunc = void (*)(const Kuru::SDK::RuntimeBindings*);
        if (const auto initRuntime = reinterpret_cast<InitRuntimeFunc>(GetProcAddress(dll, "KuruInitRuntime"))) {
            initRuntime(&Kuru::SDK::GetRuntimeBindings());
        } else {
            Log("mod missing KuruInitRuntime (built without KURU_EXPORT_MOD?): " + entry.path().string());
        }

        // Push the host API table (hook registry, later commands/log) so the mod can register.
        using InitHostApiFunc = void (*)(const Kuru::HostApi*);
        if (const auto initHostApi = reinterpret_cast<InitHostApiFunc>(GetProcAddress(dll, "KuruInitHostApi"))) {
            initHostApi(Kuru::HookEngine::Api());
        }

        const Kuru::ModInfo* info = getInfo();
        Kuru::IMod* instance = create();
        if (!instance) {
            Log("KuruCreateMod returned null: " + entry.path().string());
            FreeLibrary(dll);
            continue;
        }

        // Tag any hooks/commands the mod registers during OnLoad with this mod, so disabling it can remove them.
        Kuru::ModRegistry::SetCurrentMod(instance);
        const bool ok = instance->OnLoad();
        Kuru::ModRegistry::SetCurrentMod(nullptr);
        if (!ok) {
            Log(std::string("mod OnLoad returned false: ") + (info ? info->Name : "?"));
            destroy(instance);
            FreeLibrary(dll);
            continue;
        }

        Kuru::ModRegistry::Entry entry_;
        entry_.instance = instance;
        entry_.dll = dll;
        entry_.destroy = destroy;
        entry_.create = create;
        entry_.name = info && info->Name ? info->Name : "?";
        entry_.author = info && info->Author ? info->Author : "";
        entry_.version = info && info->Version ? info->Version : "";
        entry_.path = dllPath;
        Kuru::ModRegistry::Entries().push_back(std::move(entry_));
        Log(std::string("loaded mod: ") + (info ? info->Name : "?"));
    }
}

void UnloadAllMods() {
    auto& mods = Kuru::ModRegistry::Entries();
    // Reverse order so later mods tear down before the ones they may depend on.
    for (const auto & mod : std::views::reverse(mods)) {
        if (mod.instance) {
            mod.instance->OnUnload();
            if (mod.destroy) {
                mod.destroy(mod.instance);
            }
        }
        if (mod.dll) {
            FreeLibrary(static_cast<HMODULE>(mod.dll));
        }
    }
    mods.clear();
}

// The injector may attach before Rocket League's engine has built GNames/GObjects so poll until they exist
bool WaitForGameReady() {
    constexpr std::string_view GNamesPattern = "?? ?? ?? ?? ?? ?? 00 00 ?? ?? ?? ?? 35 25 02 00";
    for (int attempt = 0; attempt < 1800; ++attempt) {  // up to ~90s at 50ms
        const auto globals = Kuru::Common::TryInitializeRocketLeagueGlobalsFromPattern(GNamesPattern);
        if (globals.has_value() && globals->GObjects && globals->GObjects->Data &&
            globals->GObjects->Count > 0) {
            return true;
        }
        Sleep(50);
    }
    return false;
}

DWORD WINAPI HostThread(LPVOID) {
    if (!WaitForGameReady()) {
        Log("HostThread: game globals never became ready (waited ~90s); aborting");
        return 1;
    }
    if (!BootstrapGlobals()) {
        return 1;
    }
    Kuru::Variables::RegisterConsoleCommands();  // set/get/cvarlist
    LoadModsFrom(SelfDirectory() / "mods");
    // Drive each enabled mod's OnRender() inside the overlay frame.
    Kuru::Gui::SetModRenderer([]() {
        for (const Kuru::ModRegistry::Entry& mod : Kuru::ModRegistry::Entries()) {
            if (mod.instance && mod.enabled) {
                mod.instance->OnRender();
            }
        }
    });
    // Enable the ProcessEvent detour only after mods have registered their hooks
    if (gProcessEventAddress) {
        Kuru::HookEngine::Install(gProcessEventAddress);
    }
    if (kMappingMode && gVTable) {
        // dev probe
        for (const int slots[] = {64, 65, 66, 68, 69, 71, 72, 73, 74, 75, 76, 77, 78, 79}; int idx : slots) {
            const std::string label = "v" + std::to_string(idx);
            Kuru::HookEngine::Probe::Install(idx, gVTable[idx], label.c_str());
        }
        Kuru::HookEngine::Probe::RegisterConsoleCommand();
    } else {
        // All confirmed dispatch layers
        if (gProcessDelegateAddress) {
            Kuru::HookEngine::InstallProcessDelegate(gProcessDelegateAddress);        // 68 PD
        }
        if (gCallDelegateAddress) {
            Kuru::HookEngine::InstallCallDelegate(gCallDelegateAddress);              // 69 CD
        }
        if (gRemoteFunctionAddress) {
            Kuru::HookEngine::InstallProcessRemoteFunction(gRemoteFunctionAddress);   // 71 PRF
        }
        if (gVTable) {
            Kuru::HookEngine::InstallCallFunction(gVTable[kCallFunctionVTableIndex]); // 76 CF
        }
        if (gPIExact) {
            Kuru::HookEngine::InstallProcessInternal(gPIExact);                       // ProcessInternal
        } else {
            Log("ProcessInternal NOT resolved");
        }
    }
    Kuru::Gui::Install();
    return 0;
}

} // namespace

BOOL APIENTRY DllMain(const HMODULE module, const DWORD reason, const LPVOID reserved) {
    (void)reserved;

    switch (reason) {
    case DLL_PROCESS_ATTACH:
        gSelfModule = module;
        DisableThreadLibraryCalls(module);
        // Heavy work must not run inside the loader lock, so hand it off to a worker thread.
        if (HANDLE thread = CreateThread(nullptr, 0, HostThread, nullptr, 0, nullptr)) {
            CloseHandle(thread);
        }
        break;
    case DLL_PROCESS_DETACH:
        Kuru::Gui::Shutdown();
        Kuru::HookEngine::Probe::ShutdownAll();
        Kuru::HookEngine::Shutdown();
        UnloadAllMods();
        Kuru::SDK::Shutdown();
        break;
    default:
        break;
    }

    return TRUE;
}
