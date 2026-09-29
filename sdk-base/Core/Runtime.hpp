#pragma once

#include "Containers.hpp"
#include "Memory.hpp"
#include "Names.hpp"

#include <cstddef>
#include <cstdint>

namespace Kuru::SDK {

class UObject;
class UClass;
class UFunction;
class UPackage;
struct FFrame;
struct FStateFrame;

using ProcessEventFunc = void(__fastcall*)(UObject* object, UFunction* function, void* params, void* result);
using ProcessInternalFunc = void(__fastcall*)(UObject* object, FFrame& stack, void* result);
using ProcessDelegateFunc = void(__fastcall*)(UObject* object, FName delegateName, void* params, void* result);
using ProcessRemoteFunctionFunc = bool(__fastcall*)(UObject* object, UFunction* function, void* params, void* stack, UObject* subObject);
using ScriptConsoleExecFunc = bool(__fastcall*)(UObject* object, const wchar_t* command, void* outputDevice, UObject* executor);
using StaticFindObjectFunc = UObject*(__fastcall*)(UClass* objectClass, UObject* objectPackage, const wchar_t* objectName, bool exactClass);
using StaticFindObjectFastFunc = UObject*(__fastcall*)(UClass* objectClass, UObject* objectPackage, FName objectName, bool exactClass);
using StaticLoadObjectFunc = UObject*(__fastcall*)(UClass* objectClass, UObject* outer, const wchar_t* name,
    const wchar_t* filename, std::uint32_t loadFlags, void* sandbox);
using DynamicLoadObjectFunc = UObject*(__fastcall*)(const wchar_t* objectName, UClass* objectClass, bool mayFail);
using FNameConstructorFunc = void(__fastcall*)(FName* outName, const wchar_t* name, EFindName findType);
using ObjectInitializerFunc = void(__fastcall*)(UObject* object);
using MallocFunc = void*(__fastcall*)(std::uint32_t size, std::uint32_t alignment);
using ReallocFunc = void*(__fastcall*)(void* original, std::uint32_t size, std::uint32_t alignment);
using FreeFunc = void(__fastcall*)(void* original);

struct RuntimeBindings {
    TArray<FNameEntry*>* Names = nullptr;
    TArray<UObject*>* Objects = nullptr;
    FMalloc* Malloc = nullptr;
    const char* BuildDate = nullptr;
    const wchar_t* BuildId = nullptr;
    std::uint32_t BuildNumber = 0;

    ProcessEventFunc ProcessEvent = nullptr;
    ProcessInternalFunc ProcessInternal = nullptr;
    ProcessDelegateFunc ProcessDelegate = nullptr;
    ProcessRemoteFunctionFunc ProcessRemoteFunction = nullptr;
    ScriptConsoleExecFunc ScriptConsoleExec = nullptr;
    StaticFindObjectFunc StaticFindObject = nullptr;
    StaticFindObjectFastFunc StaticFindObjectFast = nullptr;
    StaticLoadObjectFunc StaticLoadObject = nullptr;
    DynamicLoadObjectFunc DynamicLoadObject = nullptr;
    FNameConstructorFunc NameConstructor = nullptr;
    ObjectInitializerFunc ObjectInitializer = nullptr;
    MallocFunc RawMalloc = nullptr;
    ReallocFunc RawRealloc = nullptr;
    FreeFunc RawFree = nullptr;
};

void SetRuntimeBindings(const RuntimeBindings& bindings) noexcept;
[[nodiscard]] const RuntimeBindings& GetRuntimeBindings() noexcept;

bool Init();
void Shutdown() noexcept;
[[nodiscard]] bool IsInitialized() noexcept;
bool ResolveRuntime();

[[nodiscard]] const char* GetBuildDate() noexcept;
[[nodiscard]] const wchar_t* GetBuildId() noexcept;
[[nodiscard]] std::uint32_t GetBuildNumber() noexcept;
[[nodiscard]] TArray<FNameEntry*>* GetGlobalNames() noexcept;
[[nodiscard]] TArray<UObject*>* GetGlobalObjects() noexcept;
[[nodiscard]] FMalloc* GetMalloc() noexcept;
[[nodiscard]] ProcessEventFunc GetProcessEvent() noexcept;
[[nodiscard]] ProcessInternalFunc GetProcessInternal() noexcept;
[[nodiscard]] ProcessDelegateFunc GetProcessDelegate() noexcept;
[[nodiscard]] ProcessRemoteFunctionFunc GetProcessRemoteFunction() noexcept;
[[nodiscard]] ScriptConsoleExecFunc GetScriptConsoleExec() noexcept;
[[nodiscard]] StaticFindObjectFunc GetStaticFindObject() noexcept;
[[nodiscard]] StaticFindObjectFastFunc GetStaticFindObjectFast() noexcept;
[[nodiscard]] StaticLoadObjectFunc GetStaticLoadObject() noexcept;
[[nodiscard]] DynamicLoadObjectFunc GetDynamicLoadObject() noexcept;
[[nodiscard]] FNameConstructorFunc GetNameConstructor() noexcept;
[[nodiscard]] ObjectInitializerFunc GetObjectInitializer() noexcept;
[[nodiscard]] FMalloc* GetMemoryAllocator() noexcept;

[[nodiscard]] void* Allocate(std::uint32_t size, std::uint32_t alignment = alignof(std::max_align_t));
[[nodiscard]] void* Reallocate(void* original, std::uint32_t size, std::uint32_t alignment = alignof(std::max_align_t));
void Deallocate(void* original) noexcept;

bool KuruInitGlobals();

} // namespace Kuru::SDK
