#include "HookEngine.hpp"

#include "Console.hpp"
#include "Gui.hpp"
#include "ModRegistry.hpp"
#include "Scanner.hpp"
#include "Storage.hpp"
#include "ThreadBridge.hpp"
#include "Variables.hpp"

#include <Core/Delegates.hpp>
#include <Core/ObjectModel.hpp>
#include <Core/Script.hpp>

#include <cstring>

#include "Hook.hpp"
#include <windows.h>

#include <algorithm>
#include <atomic>
#include <cstdio>
#include <memory>
#include <mutex>
#include <string>
#include <unordered_map>
#include <unordered_set>
#include <vector>

namespace Kuru::HookEngine {
namespace {

void Log(const std::string& message) {
    OutputDebugStringA(("[Kuru] " + message + "\n").c_str());
    Kuru::Console::AddLog(message.c_str());
}

// Diagnostic (off by default)
bool gTrace = false;


/* ProcessEvent's signature. On x64 there is a single calling convention, so a free function with
 * this signature receives the same registers (RCX=this, RDX=fn, R8=params, R9=result) as the
 * member call the game makes.
 */
using ProcessEventFn = void (*)(Kuru::SDK::UObject* caller, Kuru::SDK::UFunction* function, void* params, void* result);

ProcessEventFn gOriginalProcessEvent = nullptr;
/* Slot 76 = CallFunction: void(UObject* this, FFrame& Stack, void* Result, UFunction* Function).
 * The function being called is arg3 (R9), the caller's frame is arg1 (RDX).
 */
using CallFunctionFn = void (*)(Kuru::SDK::UObject* caller, Kuru::SDK::FFrame* stack, void* result,
                                Kuru::SDK::UFunction* function);
CallFunctionFn gOriginalCallFunction = nullptr;

/*  Interpreter opcode->handler table (resolved by the host from ProcessInternal). Each entry is a member
 * function `void (UObject::*)(FFrame&, void* Result)`. Used by the CallFunction hook to evaluate a call's
 * arguments through the game's own interpreter
*/
using NativeHandlerFn = void (*)(Kuru::SDK::UObject* context, Kuru::SDK::FFrame* stack, void* result);
NativeHandlerFn* gGNatives = nullptr;

/*  Interpreter runtime globals (the opcode handlers write these). gGPropAddr is the address the last
 * evaluated arg resolved to, read after each GNatives call to capture where a param lives, so CF hooks
 * can write edits back. The others are held for the full re-invoke path.
*/
void** gGPropAddr = nullptr;
void** gGProperty = nullptr;
void** gGPropObject = nullptr;
std::uint32_t* gGRuntimeUCFlags = nullptr;

/* Slot 70 = ProcessInternal: std::int64_t(void* rcx, void* frame, void* r8, void* r9).
 * ProcessInternal is non-virtual. RL's fork FFrame differs from stock UE3 (fields shifted +0x10):
 * the executing UFunction is Stack.Node at frame+0x18, the Locals buffer pointer is at frame+0x30
 * Universal safe shape: 4 register args forwarded + int64 return (it returns a value, a void detour clobbers RAX like PRF).
 */
using ProcessInternalFrameFn = std::int64_t (*)(void* rcx, void* frame, void* r8, void* r9);
ProcessInternalFrameFn gOriginalProcessInternal = nullptr;
constexpr std::size_t kProcessInternalFuncOffset = offsetof(Kuru::SDK::FFrame, Node);   // Stack.Node = executing UFunction
constexpr std::size_t kProcessInternalLocalsOffset = offsetof(Kuru::SDK::FFrame, Locals);  // params/Locals pointer
Kuru::THook gProcessEventHook;
Kuru::THook gProcessInternalHook;
Kuru::THook gCallFunctionHook;

struct HookEntry {
    Kuru::HookCallbackFn callback;
    void* user;
    Kuru::HookHandle handle;
    void* owner;              // the mod that registered it (remove all of a mod's hooks on disabled)
    Kuru::HookDispatch dispatch;  // which layer this hook wants
};

class HookMap {
public:
    using value_type = std::pair<Kuru::SDK::UFunction*, std::vector<HookEntry>>;
    using iterator = std::vector<value_type>::iterator;
    using const_iterator = std::vector<value_type>::const_iterator;

    iterator begin() noexcept { return entries_.begin(); }
    iterator end() noexcept { return entries_.end(); }
    [[nodiscard]] const_iterator begin() const noexcept { return entries_.begin(); }
    [[nodiscard]] const_iterator end() const noexcept { return entries_.end(); }

    [[nodiscard]] const_iterator find(Kuru::SDK::UFunction* key) const {
        const auto it = std::lower_bound(entries_.begin(), entries_.end(), key, Less);
        return (it != entries_.end() && it->first == key) ? it : entries_.end();
    }
    [[nodiscard]] std::size_t count(Kuru::SDK::UFunction* key) const {
        return find(key) != entries_.end() ? 1 : 0;
    }
    std::vector<HookEntry>& operator[](Kuru::SDK::UFunction* key) {
        auto it = std::lower_bound(entries_.begin(), entries_.end(), key, Less);
        if (it == entries_.end() || it->first != key) {
            it = entries_.insert(it, value_type{key, {}});
        }
        return it->second;
    }

private:
    static bool Less(const value_type& entry, Kuru::SDK::UFunction* key) { return entry.first < key; }
    std::vector<value_type> entries_;  // sorted ascending by first (the UFunction*)
};
struct HookTables {
    HookMap pre;
    HookMap post;
};

/* Copy-on-write hook tables: detours do one lock-free snapshot load, writers clone, mutate, and
 * publish under gWriteMutex. gTablesOwner keeps the active snapshot alive, and gRetiredTables keeps
 * old snapshots alive for in-flight detours.
 */
std::shared_ptr<const HookTables> gTablesOwner = std::make_shared<const HookTables>();
std::atomic<const HookTables*> gTablesRaw{gTablesOwner.get()};
std::vector<std::shared_ptr<const HookTables>> gRetiredTables;
std::mutex gWriteMutex;
Kuru::HookHandle gNextHandle = 1;

/* Per-function reentrancy stack (game thread). Only the function's canonical layer (see
 * ShouldFireAny) pushes onto it, so an Any-hook fires once per call even under recursion.
 * EnterActive() returns true only on first entry; nested redispatch returns false.
 */
thread_local std::vector<Kuru::SDK::UFunction*> tlActive;

bool EnterActive(Kuru::SDK::UFunction* function) {
    for (Kuru::SDK::UFunction* active : tlActive) {
        if (active == function) {
            return false;
        }
    }
    tlActive.push_back(function);
    return true;
}

void LeaveActive(Kuru::SDK::UFunction* function, bool entered) {
    if (!entered) {
        return;
    }
    for (auto it = tlActive.end(); it != tlActive.begin();) {
        --it;
        if (*it == function) {
            tlActive.erase(it);
            return;
        }
    }
}


Kuru::HookDispatch CanonicalLayer(Kuru::SDK::UFunction* function) {
    const bool isNative = function->iNative != 0 ||
                          (function->FunctionFlags & Kuru::SDK::FUNC_Native) != 0;
    const bool hasBody = (function->FunctionFlags & Kuru::SDK::FUNC_Defined) != 0;
    return (!isNative && hasBody) ? Kuru::HookDispatch::ProcessInternal
                                  : Kuru::HookDispatch::CallFunction;
}

// True when Any hooks should fire at `layer` for this call: `layer` is the function's canonical layer
bool ShouldFireAny(Kuru::SDK::UFunction* function, Kuru::HookDispatch layer) {
    return function && CanonicalLayer(function) == layer && EnterActive(function);
}


bool SafeInvokeHook(Kuru::HookCallbackFn callback, Kuru::HookContext* context, void* user) {
    __try {
        callback(context, user);
        return true;
    } __except (EXCEPTION_EXECUTE_HANDLER) {
        return false;
    }
}

void DispatchFrom(const HookMap& map, Kuru::HookContext& context, bool fireAny) {
    const auto it = map.find(context.function);
    if (it == map.end()) {
        return;
    }
    for (const HookEntry& entry : it->second) {

        const bool fire = entry.dispatch == context.dispatch ||
                          (entry.dispatch == Kuru::HookDispatch::Any && fireAny);
        if (!fire) {
            continue;
        }
        // Fault isolation: a throwing or faulting mod callback must not unwind into the game's call stack.
        if (!SafeInvokeHook(entry.callback, &context, entry.user)) {
            Log(std::string("hook callback faulted (suppressed) on ") +
                (context.function ? context.function->GetName() : std::string("?")));
        }
    }
}

// True if the function has any pre/post hook
bool HasHooks(const HookTables* tables, Kuru::SDK::UFunction* function) {
    return function && (tables->pre.count(function) != 0 || tables->post.count(function) != 0);
}


// Serialized writers: clone the current tables, hand the clone to `mutator`, publish atomically.
template <typename Mutator>
void MutateTables(Mutator&& mutator) {
    std::scoped_lock lock(gWriteMutex);
    auto next = std::make_shared<HookTables>(*gTablesOwner);  // clone current, mutate, publish
    mutator(*next);
    gRetiredTables.push_back(gTablesOwner);  // an in-flight detour may still read the old pointer
    gTablesOwner = next;
    gTablesRaw.store(next.get(), std::memory_order_release);
}

// Flip gTrace (declared above) to true to debug hook resolution.
void TraceProcessEvent(Kuru::SDK::UFunction* function) {
    static bool firstCall = true;
    if (firstCall) {
        firstCall = false;
        Log("ProcessEvent detour is FIRING (interception works)");
    }
    static std::unordered_map<Kuru::SDK::UFunction*, bool> classified;
    if (classified.find(function) == classified.end()) {
        const std::string path = function ? function->GetPathName() : std::string();
        // Widen the net
        const bool interesting = path.find("Goal") != std::string::npos ||
                                 path.find("Score") != std::string::npos ||
                                 path.find("ResetBall") != std::string::npos;
        classified.emplace(function, interesting);
        if (interesting) {
            Log("PE sees: " + path);
        }
    }
}

void DetourProcessEvent(Kuru::SDK::UObject* caller, Kuru::SDK::UFunction* function, void* params, void* result) {
    // Game-thread tick: runs queued Execute tasks, timeouts, and mod OnUpdate once per frame.
    Kuru::ThreadBridge::PumpFromGameThread();
    if (gTrace) {
        TraceProcessEvent(function);
    }
    if (Kuru::Scanner::IsEnabled()) {
        Kuru::Scanner::Record(caller, function, params);
    }
    // One lock-free snapshot for the whole call so pre/post see a consistent view.
    const HookTables* tables = gTablesRaw.load(std::memory_order_acquire);
    if (!HasHooks(tables, function)) {  // hot path: unhooked calls skip dispatch entirely
        if (gOriginalProcessEvent) gOriginalProcessEvent(caller, function, params, result);
        return;
    }
    const bool fireAny = ShouldFireAny(function, Kuru::HookDispatch::ProcessEvent);
    Kuru::HookContext context{Kuru::HookDispatch::ProcessEvent, caller, function,
                              params, result, nullptr, false};
    DispatchFrom(tables->pre, context, fireAny);
    if (!context.block && gOriginalProcessEvent) {  // a Pre hook can Block() the original
        gOriginalProcessEvent(caller, function, params, result);
    }
    DispatchFrom(tables->post, context, fireAny);
    LeaveActive(function, fireAny);
}

// One argument's location, captured during eval so a hook's edit can be written back.
struct ArgSlot {
    void* addr;            // where the arg actually lives (GPropAddr), null for a literal/expression
    std::uint16_t offset;  // its offset in the params buffer
    std::uint16_t size;    // element size
    bool isOut;            // CPF_OutParm, the function writes this back (re-read it after the call)
};

int EvaluateCallArgs(Kuru::SDK::UFunction* function, Kuru::SDK::FFrame* stack, std::uint8_t* buffer,
                     std::size_t bufSize, ArgSlot* slots, int maxSlots, std::uint8_t** outPostArgsCode) {
    if (outPostArgsCode) {
        *outPostArgsCode = nullptr;
    }
    if (!gGNatives || !function || !stack || !stack->Code || !stack->Object) {
        return -1;
    }
    constexpr std::uint64_t CPF_Parm = 0x80;
    constexpr std::uint64_t CPF_OutParm = 0x100;
    constexpr std::uint64_t CPF_ReturnParm = 0x400;
    constexpr std::uint32_t RUC_SkippedOptionalParm = 0x2;
    std::uint8_t* const savedCode = stack->Code;
    int count = 0;
    __try {
        for (Kuru::SDK::UField* field = function->Children; field; field = field->Next) {
            auto* prop = reinterpret_cast<Kuru::SDK::UProperty*>(field);
            const std::uint64_t flags = prop->PropertyFlags;
            if ((flags & CPF_Parm) == 0 || (flags & CPF_ReturnParm) != 0) {
                continue;  // only the actual arguments (params, excluding the return slot)
            }
            const std::size_t off = static_cast<std::size_t>(prop->Offset);
            const std::size_t sz = static_cast<std::size_t>(prop->ElementSize);
            if (off + sz > bufSize) {
                break;  // never write past the buffer
            }
            if (gGRuntimeUCFlags) {
                *gGRuntimeUCFlags &= ~RUC_SkippedOptionalParm;  // mirror CallFunction's per-param reset
            }
            if (gGPropAddr) {
                *gGPropAddr = nullptr;  // clear so we can tell an l-value (address set) from a literal
            }
            const std::uint8_t op = *stack->Code++;             // read this arg's opcode, advance Code
            gGNatives[op](stack->Object, stack, buffer + off);  // game evaluates the arg into the buffer
            if (count < maxSlots) {
                slots[count].addr = gGPropAddr ? *gGPropAddr : nullptr;  // where the arg really lives
                slots[count].offset = static_cast<std::uint16_t>(off);
                slots[count].size = static_cast<std::uint16_t>(sz);
                slots[count].isOut = (flags & CPF_OutParm) != 0;
                ++count;
            }
        }
        if (outPostArgsCode) {
            *outPostArgsCode = stack->Code;  // now at EndFunctionParms (before rewind)
        }
        stack->Code = savedCode;                                // rewind for the real call
        return count;
    } __except (EXCEPTION_EXECUTE_HANDLER) {
        stack->Code = savedCode;                                // rewind even on fault
        return -1;
    }
}


bool ReInvokeNative(Kuru::SDK::UObject* caller, Kuru::SDK::UFunction* function,
                    std::uint8_t* buffer, void* result) {
    if (!gOriginalProcessEvent) {
        return false;
    }
    __try {
        gOriginalProcessEvent(caller, function, buffer, result);
        return true;
    } __except (EXCEPTION_EXECUTE_HANDLER) {
        return false;
    }
}

void DetourCallFunction(Kuru::SDK::UObject* caller, Kuru::SDK::FFrame* stack, void* result,
                        Kuru::SDK::UFunction* function) {
                            /*
    static bool firstCall = true;
    if (firstCall) {
        firstCall = false;
        Log("CallFunction (slot 76) detour is FIRING");
    }
    */
    const HookTables* tables = gTablesRaw.load(std::memory_order_acquire);
    if (!HasHooks(tables, function)) {
        if (gOriginalCallFunction) gOriginalCallFunction(caller, stack, result, function);
        return;
    }

    const bool fireAny = ShouldFireAny(function, Kuru::HookDispatch::CallFunction);

    std::uint8_t argBuffer[1024];
    std::uint8_t argOriginal[1024];
    ArgSlot slots[32];
    int slotCount = 0;
    std::size_t argSize = 0;
    std::uint8_t* postArgsCode = nullptr;
    void* params = nullptr;
    if (gGNatives && CanonicalLayer(function) == Kuru::HookDispatch::CallFunction) {
        argSize = function->ParmsSize;
        if (argSize > sizeof(argBuffer)) {
            argSize = sizeof(argBuffer);
        }
        std::memset(argBuffer, 0, argSize);
        slotCount = EvaluateCallArgs(function, stack, argBuffer, argSize, slots, 32, &postArgsCode);
        if (slotCount >= 0) {
            params = argBuffer;  // holds the args' pre-call values, valid for this detour's lifetime
            std::memcpy(argOriginal, argBuffer, argSize);  // baseline to detect Pre-hook edits
        } else {
            slotCount = 0;
        }
    }
    Kuru::HookContext context{Kuru::HookDispatch::CallFunction, caller, function,
                              params, result, stack, false};

    // --- PRE hooks ---
    DispatchFrom(tables->pre, context, fireAny);

    // --- CANCEL (a Pre hook called Block())
    if (context.block && postArgsCode) {
        stack->Code = postArgsCode + 1;
        if (Kuru::SDK::UProperty* ret = function->GetReturnProperty(); ret && result) {
            std::memset(result, 0, static_cast<std::size_t>(ret->ElementSize));  // no garbage return
        }
        DispatchFrom(tables->post, context, fireAny);
        LeaveActive(function, fireAny);
        return;
    }

    // --- RE-INVOKE ---
    constexpr std::uint32_t FUNC_Native = 0x400;
    const bool reInvokeEligible =
        params && postArgsCode &&                        // args materialised + we know where to resume
        (function->FunctionFlags & FUNC_Native) != 0 &&  // true native (Func is the C++ impl)
        function->iNative == 0 &&                        // not a core indexed native
        function->Script.Count > 0 &&                    // has NativeParm stubs -> ProcessEvent-callable
        function->ParmsSize <= sizeof(argBuffer);        // ProcessEvent memcpy's ParmsSize from our buffer
    if (reInvokeEligible) {
        // Native reads in-params (incl. any Pre-hook edits, even to literals) from argBuffer then writes out-params into argBuffer and the return into result.
        if (ReInvokeNative(caller, function, argBuffer, result)) {
            /*
            static bool firstReInvoke = true;
            if (firstReInvoke) {
                firstReInvoke = false;
                Log("re-invoke path FIRED for a native via ProcessEvent");
            }
            */
            for (int i = 0; i < slotCount; ++i) {  // push out-params to the caller's real vars
                if (slots[i].isOut && slots[i].addr) {
                    std::memcpy(slots[i].addr, argBuffer + slots[i].offset, slots[i].size);
                }
            }
            stack->Code = postArgsCode + 1;                // skip the inline call
            std::memcpy(argOriginal, argBuffer, argSize);  // baseline to detect Post-hook edits
            DispatchFrom(tables->post, context, fireAny);
            for (int i = 0; i < slotCount; ++i) {  // EDIT-OUT: Post edits -> caller vars (return is in result)
                if (slots[i].isOut && slots[i].addr &&
                    std::memcmp(argBuffer + slots[i].offset, argOriginal + slots[i].offset, slots[i].size) != 0) {
                    std::memcpy(slots[i].addr, argBuffer + slots[i].offset, slots[i].size);
                }
            }
            LeaveActive(function, fireAny);
            return;
        }
        // Fault: the native almost certainly didn't run (a bad frame faults on entry), so fall through to  the inline write-back path below, which re-reads args from bytecode and calls it for real.
        static bool warnedReInvoke = false;
        if (!warnedReInvoke) {
            warnedReInvoke = true;
            Log("re-invoke faulted, falling back to inline write-back for this native");
        }
    }

    // --- EDIT-IN write-back ---
    if (params) {
        for (int i = 0; i < slotCount; ++i) {
            if (slots[i].addr &&
                std::memcmp(argBuffer + slots[i].offset, argOriginal + slots[i].offset, slots[i].size) != 0) {
                std::memcpy(slots[i].addr, argBuffer + slots[i].offset, slots[i].size);
            }
        }
    }

    // --- THE CALL ---
    if (gOriginalCallFunction) {
        gOriginalCallFunction(caller, stack, result, function);
    }

    // --- CATCH-OUT ---
    if (params) {
        for (int i = 0; i < slotCount; ++i) {
            if (slots[i].isOut && slots[i].addr) {
                std::memcpy(argBuffer + slots[i].offset, slots[i].addr, slots[i].size);
            }
        }
        std::memcpy(argOriginal, argBuffer, argSize);  // new baseline to detect Post-hook edits
    }

    // --- POST hooks ---
    DispatchFrom(tables->post, context, fireAny);

    // --- EDIT-OUT write-back ---
    if (params) {
        for (int i = 0; i < slotCount; ++i) {
            if (slots[i].addr &&
                std::memcmp(argBuffer + slots[i].offset, argOriginal + slots[i].offset, slots[i].size) != 0) {
                std::memcpy(slots[i].addr, argBuffer + slots[i].offset, slots[i].size);
            }
        }
    }
    LeaveActive(function, fireAny);
}

// Stack.Node is the function, Stack.Locals holds its params.
std::int64_t DetourProcessInternal(void* rcx, void* frame, void* r8, void* r9) {
    /*
    static bool firstCall = true;
    if (firstCall) {
        firstCall = false;
        Log("ProcessInternal detour is FIRING");
    }
    */
    Kuru::SDK::UFunction* function = nullptr;
    if (frame) {
        function = *reinterpret_cast<Kuru::SDK::UFunction**>(
            reinterpret_cast<char*>(frame) + kProcessInternalFuncOffset);
    }
    const HookTables* tables = gTablesRaw.load(std::memory_order_acquire);
    if (!HasHooks(tables, function)) {
        return gOriginalProcessInternal ? gOriginalProcessInternal(rcx, frame, r8, r9) : 0;
    }
    void* params = *reinterpret_cast<void**>(reinterpret_cast<char*>(frame) + kProcessInternalLocalsOffset);
    void* result = r8;
    const bool fireAny = ShouldFireAny(function, Kuru::HookDispatch::ProcessInternal);
    Kuru::HookContext context{Kuru::HookDispatch::ProcessInternal, reinterpret_cast<Kuru::SDK::UObject*>(rcx),
                              function, params, result, reinterpret_cast<Kuru::SDK::FFrame*>(frame), false};
    DispatchFrom(tables->pre, context, fireAny);
    std::int64_t ret = 0;
    if (!context.block && gOriginalProcessInternal) {
        ret = gOriginalProcessInternal(rcx, frame, r8, r9);
    }
    DispatchFrom(tables->post, context, fireAny);
    LeaveActive(function, fireAny);
    return ret;
}


std::unordered_set<void*> gOverrideWarnOptOut;
std::mutex gOptOutMutex;

void SuppressOverrideWarnings() {
    std::scoped_lock lock(gOptOutMutex);
    gOverrideWarnOptOut.insert(Kuru::ModRegistry::CurrentMod());  // the mod loading right now
}

void WarnIfOverridden(Kuru::SDK::UFunction* hooked, const char* name) {
    {
        std::scoped_lock lock(gOptOutMutex);
        if (gOverrideWarnOptOut.count(Kuru::ModRegistry::CurrentMod()) != 0) {
            return;
        }
    }
    Kuru::SDK::UObject* baseOuter = hooked->GetOuter();          // the class the function is defined on
    Kuru::SDK::UClass* funcClass = Kuru::SDK::UFunction::StaticClass();
    Kuru::SDK::TArray<Kuru::SDK::UObject*>* objects = Kuru::SDK::UObject::GetGlobalObjects();
    if (!baseOuter || !funcClass || !objects) {
        return;
    }
    const auto* baseStruct = reinterpret_cast<const Kuru::SDK::UStruct*>(baseOuter);
    const Kuru::SDK::FName leaf = hooked->GetFName();
    for (Kuru::SDK::UObject* obj : *objects) {
        if (!obj || obj == hooked || !obj->IsA(funcClass)) {
            continue;
        }
        const Kuru::SDK::FName other = obj->GetFName();
        if (other.GetIndex() != leaf.GetIndex() || other.GetNumber() != leaf.GetNumber()) {
            continue;
        }
        Kuru::SDK::UObject* objOuter = obj->GetOuter();
        const auto* objStruct = reinterpret_cast<const Kuru::SDK::UStruct*>(objOuter);
        if (objStruct && objStruct != baseStruct && objStruct->IsChildOf(baseStruct)) {
            Log(std::string("hook '") + name +
                "' targets a base class, but RL likely dispatches an override: '" +
                obj->GetPathName() + "'. Hook that exact name instead, or call "
                "Kuru::Hooks::SuppressOverrideWarnings() in your mod to silence this.");
            return;  // one hint is enough
        }
    }
}

Kuru::HookHandle RegisterHook(const char* functionFullName, Kuru::HookTiming timing,
                              Kuru::HookDispatch dispatch, Kuru::HookCallbackFn cb, void* user) {
    if (!functionFullName || !cb) {
        return 0;
    }
    Kuru::SDK::UFunction* function = Kuru::SDK::UObject::FindFunction(functionFullName);
    if (!function) {
        Log(std::string("RegisterHook: function not found: ") + functionFullName);
        return 0;
    }
    WarnIfOverridden(function, functionFullName);
    void* owner = Kuru::ModRegistry::CurrentMod();
    Kuru::HookHandle handle = 0;
    MutateTables([&](HookTables& tables) {
        handle = gNextHandle++;
        HookMap& map = (timing == Kuru::HookTiming::Pre) ? tables.pre : tables.post;
        map[function].push_back({cb, user, handle, owner, dispatch});
    });
    Log(std::string("hook registered [") + (timing == Kuru::HookTiming::Pre ? "pre" : "post") +
        "] " + functionFullName);
    return handle;
}

void RemoveHook(Kuru::HookHandle handle) {
    MutateTables([&](HookTables& tables) {
        for (HookMap* map : {&tables.pre, &tables.post}) {
            for (auto& [function, entries] : *map) {
                entries.erase(std::remove_if(entries.begin(), entries.end(),
                                             [handle](const HookEntry& e) { return e.handle == handle; }),
                              entries.end());
            }
        }
    });
}

// ---- Module event bus ---
struct Listener {
    Kuru::ListenerThunkFn thunk;
    void* user;
    Kuru::HookHandle handle;
    void* owner;       // registering mod (for RemoveHooksForMod cleanup)
    int priority;      // higher fires first
    std::string filter;
};
std::unordered_map<std::string, std::vector<Listener>> gListeners;  // keyed by event identifier
std::mutex gListenerMutex;

// SEH-guarded listener call (same rationale as SafeInvokeHook: contain access violations from mod code).
bool SafeInvokeListener(Kuru::ListenerThunkFn thunk, void* payload, void* user) {
    __try {
        thunk(payload, user);
        return true;
    } __except (EXCEPTION_EXECUTE_HANDLER) {
        return false;
    }
}
std::atomic<Kuru::HookHandle> gNextListenerHandle{0x8000000000000000ull};  // high bit: distinct from hook handles

Kuru::HookHandle RegisterListener(const char* identifier, const char* listenerName,
                                  Kuru::ListenerThunkFn thunk, void* user, const char* filter, int priority) {
    if (!identifier || !thunk) {
        return 0;
    }
    const Kuru::HookHandle handle = gNextListenerHandle++;
    std::scoped_lock lock(gListenerMutex);
    auto& vec = gListeners[identifier];
    vec.push_back({thunk, user, handle, Kuru::ModRegistry::CurrentMod(), priority, filter ? filter : ""});
    std::stable_sort(vec.begin(), vec.end(),
                     [](const Listener& a, const Listener& b) { return a.priority > b.priority; });
    (void)listenerName;
    return handle;
}

void UnregisterListener(Kuru::HookHandle handle) {
    std::scoped_lock lock(gListenerMutex);
    for (auto& [identifier, vec] : gListeners) {
        vec.erase(std::remove_if(vec.begin(), vec.end(),
                                 [handle](const Listener& l) { return l.handle == handle; }),
                  vec.end());
    }
}

void DispatchEvent(const char* identifier, const char* filter, void* payload) {
    if (!identifier) {
        return;
    }
    const std::string want = filter ? filter : "";
    std::vector<Listener> matched;
    {
        std::scoped_lock lock(gListenerMutex);
        const auto it = gListeners.find(identifier);
        if (it == gListeners.end()) {
            return;
        }
        for (const Listener& l : it->second) {
            if (l.filter.empty() || l.filter == want) {  // a listener with no filter matches every dispatch
                matched.push_back(l);
            }
        }
    }
    std::vector<Kuru::HookHandle> faulted;
    for (const Listener& l : matched) {  // fire outside the lock: a listener may register/dispatch reentrantly
        if (!SafeInvokeListener(l.thunk, payload, l.user)) {
            Log(std::string("event listener faulted for ") + identifier + ", removing it");
            faulted.push_back(l.handle);  // a listener that access-violates once will keep doing it: evict it
        }
    }
    if (!faulted.empty()) {
        std::scoped_lock lock(gListenerMutex);
        const auto it = gListeners.find(identifier);
        if (it != gListeners.end()) {
            auto& vec = it->second;
            vec.erase(std::remove_if(vec.begin(), vec.end(),
                                     [&faulted](const Listener& l) {
                                         return std::find(faulted.begin(), faulted.end(), l.handle) !=
                                                faulted.end();
                                     }),
                      vec.end());
        }
    }
}

Kuru::HostApi gApi{Kuru::HostApiVersion, &RegisterHook, &RemoveHook,
                   &Kuru::Console::AddLogTagged, &Kuru::Console::RegisterCommand,
                   &Kuru::Gui::HostGetImGuiContext, &Kuru::Gui::HostGetImGuiAllocators,
                   &Kuru::ThreadBridge::HostExecute, &Kuru::ThreadBridge::HostSetTimeout,
                   &Kuru::ThreadBridge::HostIsGameThread,
                   &Kuru::Storage::HostGetKuruDir, &Kuru::Storage::HostGetGameDir,
                   &Kuru::Storage::HostGetModDataDir,
                   &Kuru::Variables::HostRegisterCVar, &Kuru::Variables::GetCVar,
                   &Kuru::Variables::SetCVar, &SuppressOverrideWarnings,
                   &RegisterListener, &UnregisterListener, &DispatchEvent};

} // namespace

bool Install(void* processEventAddress) {
    if (!processEventAddress) {
        Log("hook install failed: null ProcessEvent address");
        return false;
    }
    if (!gProcessEventHook.Install(processEventAddress, reinterpret_cast<void*>(&DetourProcessEvent),
                                   reinterpret_cast<void**>(&gOriginalProcessEvent))) {
        Log("hook install failed: ProcessEvent");
        return false;
    }
    Log("ProcessEvent detour installed");
    return true;
}

bool InstallProcessInternal(void* address) {
    if (!gProcessInternalHook.Install(address, reinterpret_cast<void*>(&DetourProcessInternal),
                                      reinterpret_cast<void**>(&gOriginalProcessInternal))) {
        Log("ProcessInternal hook install failed");
        return false;
    }
    Log("ProcessInternal detour installed");
    return true;
}

bool InstallCallFunction(void* address) {
    if (!gCallFunctionHook.Install(address, reinterpret_cast<void*>(&DetourCallFunction),
                                   reinterpret_cast<void**>(&gOriginalCallFunction))) {
        Log("CallFunction hook install failed");
        return false;
    }
    Log("CallFunction (slot 76) detour installed");
    return true;
}

void SetGNatives(void* table) {
    gGNatives = reinterpret_cast<NativeHandlerFn*>(table);
}

void SetInterpreterGlobals(void* gProperty, void* gPropAddr, void* gPropObject, void* gRuntimeUCFlags) {
    gGProperty = reinterpret_cast<void**>(gProperty);
    gGPropAddr = reinterpret_cast<void**>(gPropAddr);
    gGPropObject = reinterpret_cast<void**>(gPropObject);
    gGRuntimeUCFlags = reinterpret_cast<std::uint32_t*>(gRuntimeUCFlags);
}

void Shutdown() {
    // Removing the hooks releases their MinHook references
    gProcessEventHook.Remove();
    gProcessInternalHook.Remove();
    gCallFunctionHook.Remove();
}

const Kuru::HostApi* Api() {
    return &gApi;
}

void RemoveHooksForMod(void* mod) {
    MutateTables([&](HookTables& tables) {
        for (HookMap* map : {&tables.pre, &tables.post}) {
            for (auto& [function, entries] : *map) {
                entries.erase(std::remove_if(entries.begin(), entries.end(),
                                             [mod](const HookEntry& e) { return e.owner == mod; }),
                              entries.end());
            }
        }
    });
    {
        std::scoped_lock lock(gListenerMutex);  // drop this mod's event listeners too
        for (auto& [identifier, vec] : gListeners) {
            vec.erase(std::remove_if(vec.begin(), vec.end(),
                                     [mod](const Listener& l) { return l.owner == mod; }),
                      vec.end());
        }
    }
    std::scoped_lock lock(gOptOutMutex);
    gOverrideWarnOptOut.erase(mod);  // reset the opt-out so a reloaded mod warns again
}

} // namespace Kuru::HookEngine
