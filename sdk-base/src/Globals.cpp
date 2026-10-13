#include <Core/Globals.hpp>

#include <stdexcept>
#include <cstring>

namespace Kuru::SDK {
namespace {
RuntimeBindings gBindings;
bool gInitialized = false;
}

TArray<FNameEntry*>* GNames = nullptr;
TArray<UObject*>* GObjects = nullptr;
FMalloc* GMalloc = nullptr;
const char* BuildDate = nullptr;
const wchar_t* GPsyonixBuildID = nullptr;
std::uint32_t GPsyonixBuildNumber = 0;
ProcessEventFunc ProcessEvent = nullptr;
ProcessInternalFunc ProcessInternal = nullptr;
ProcessDelegateFunc ProcessDelegate = nullptr;

void SetRuntimeBindings(const RuntimeBindings& bindings) noexcept {
    gBindings = bindings;
    GNames = bindings.Names;
    GObjects = bindings.Objects;
    GMalloc = bindings.Malloc;
    BuildDate = bindings.BuildDate;
    GPsyonixBuildID = bindings.BuildId;
    GPsyonixBuildNumber = bindings.BuildNumber;
    ProcessEvent = bindings.ProcessEvent;
    ProcessInternal = bindings.ProcessInternal;
    ProcessDelegate = bindings.ProcessDelegate;
    gInitialized = true;
}

const RuntimeBindings& GetRuntimeBindings() noexcept {
    return gBindings;
}

bool Init() {
    gInitialized = true;
    return true;
}

void Shutdown() noexcept {
    SetRuntimeBindings(RuntimeBindings{});
    gInitialized = false;
}

bool IsInitialized() noexcept {
    return gInitialized;
}

bool ResolveRuntime() {
    return IsInitialized();
}

const char* GetBuildDate() noexcept { return BuildDate; }
const wchar_t* GetBuildId() noexcept { return GPsyonixBuildID; }
std::uint32_t GetBuildNumber() noexcept { return GPsyonixBuildNumber; }
TArray<FNameEntry*>* GetGlobalNames() noexcept { return GNames; }
TArray<UObject*>* GetGlobalObjects() noexcept { return GObjects; }
FMalloc* GetMalloc() noexcept { return GMalloc; }
ProcessEventFunc GetProcessEvent() noexcept { return ProcessEvent; }
ProcessInternalFunc GetProcessInternal() noexcept { return ProcessInternal; }
ProcessDelegateFunc GetProcessDelegate() noexcept { return ProcessDelegate; }
FNameConstructorFunc GetNameConstructor() noexcept { return gBindings.NameConstructor; }
ObjectInitializerFunc GetObjectInitializer() noexcept { return gBindings.ObjectInitializer; }
FMalloc* GetMemoryAllocator() noexcept { return GMalloc; }

void* Allocate(std::uint32_t size, std::uint32_t alignment) {
    if (gBindings.RawMalloc) {
        return gBindings.RawMalloc(size, alignment);
    }
    if (GMalloc) {
        return GMalloc->Malloc(size, alignment);
    }
    throw std::runtime_error("Kuru SDK game allocator is unavailable (Allocate)");
}

void* Reallocate(void* original, std::uint32_t size, std::uint32_t alignment) {
    if (gBindings.RawRealloc) {
        return gBindings.RawRealloc(original, size, alignment);
    }
    if (GMalloc) {
        return GMalloc->Realloc(original, size, alignment);
    }
    throw std::runtime_error("Kuru SDK game allocator is unavailable (Reallocate)");
}

void Deallocate(void* original) {
    if (!original) {
        return;
    }
    if (gBindings.RawFree) {
        gBindings.RawFree(original);
        return;
    }
    if (GMalloc) {
        GMalloc->Free(original);
        return;
    }
    throw std::runtime_error("Kuru SDK game allocator is unavailable (Deallocate)");
}

bool KuruInitGlobals() {
    return Init();
}

} // namespace Kuru::SDK
