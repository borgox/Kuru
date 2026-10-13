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
[[nodiscard]] FNameConstructorFunc GetNameConstructor() noexcept;
[[nodiscard]] ObjectInitializerFunc GetObjectInitializer() noexcept;
[[nodiscard]] FMalloc* GetMemoryAllocator() noexcept;

// Engine-compatible allocation only. Missing allocator bindings throw; there is no CRT fallback.
[[nodiscard]] void* Allocate(std::uint32_t size, std::uint32_t alignment = alignof(std::max_align_t));
[[nodiscard]] void* Reallocate(void* original, std::uint32_t size, std::uint32_t alignment = alignof(std::max_align_t));
void Deallocate(void* original);

bool KuruInitGlobals();

} // namespace Kuru::SDK
