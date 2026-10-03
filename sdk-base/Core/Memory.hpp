#pragma once

#include <cstddef>
#include <cstdint>

namespace Kuru::SDK {

struct FOutputDevice;  // defined in Script.hpp (FFrame's base), used here only by reference

enum class ECacheBehaviour : std::uint32_t {
    WriteCombine,
    WriteBack,
};

struct FMemoryAllocationStats {
    std::uint64_t TotalUsed = 0;
    std::uint64_t TotalAllocated = 0;
    std::uint64_t CPUUsed = 0;
    std::uint64_t CPUAllocated = 0;
    std::uint64_t GPUUsed = 0;
    std::uint64_t GPUAllocated = 0;
};

class FMalloc {
public:
    virtual ~FMalloc() = default;
    virtual std::uint32_t QuantizeSize(std::uint32_t size, std::uint32_t alignment) {
        (void)alignment;
        return size;
    }
    virtual void* Malloc(std::uint32_t size, std::uint32_t alignment) = 0;
    virtual void* Realloc(void* original, std::uint32_t size, std::uint32_t alignment) = 0;
    virtual void Free(void* original) = 0;
    virtual void* PhysicalAlloc(std::uint32_t count, ECacheBehaviour cacheBehaviour = ECacheBehaviour::WriteCombine) {
        (void)count;
        (void)cacheBehaviour;
        return nullptr;
    }
    virtual void PhysicalFree(void* original) { (void)original; }
    virtual bool Exec(const wchar_t* cmd, FOutputDevice& out) {
        (void)cmd;
        (void)out;
        return false;
    }
    virtual void Tick(float deltaTime) { (void)deltaTime; }
    [[nodiscard]] virtual bool IsInternallyThreadSafe() const { return false; }
    virtual void GetAllocationInfo(FMemoryAllocationStats& memStats) { (void)memStats; }
    virtual void DumpAllocations(FOutputDevice& out) { (void)out; }
    [[nodiscard]] virtual bool ValidateHeap() { return true; }
    virtual void CheckMemoryFragmentationLevel(FOutputDevice& out) { (void)out; }
    virtual bool TrimMemory(std::size_t reservePad, bool showStats = false) {
        (void)reservePad;
        (void)showStats;
        return false;
    }
    virtual bool GetAllocationSize(void* original, std::uint32_t& sizeOut) {
        (void)original;
        (void)sizeOut;
        return false;
    }
};

struct FMemStackBase {
    void* Top = nullptr;
    void* End = nullptr;
};

struct FMemMark {
    FMemStackBase* Stack = nullptr;
    void* SavedTop = nullptr;
    void* SavedEnd = nullptr;
};

} // namespace Kuru::SDK
