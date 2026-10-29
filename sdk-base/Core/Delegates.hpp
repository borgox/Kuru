#pragma once

#include "Names.hpp"
#include "ObjectModel.hpp"

namespace Kuru::SDK {

// A single delegate binding (UE3 FScriptDelegate): the bound object plus the function name on it, 16 bytes on 64-bit. This is the element a multicast invocation list holds.
struct FScriptDelegate {
    UObject* Object = nullptr;   // 0x00
    FName FunctionName;          // 0x08

    [[nodiscard]] bool IsBound() const noexcept { return Object != nullptr && !FunctionName.IsNone(); }
    void BindUFunction(UObject* object, FName functionName) noexcept {
        Object = object;
        FunctionName = functionName;
    }
    void Unbind() noexcept { Clear(); }
    void Clear() noexcept {
        Object = nullptr;
        FunctionName = FName();
    }
    void ProcessDelegate(void* params, void* result = nullptr) const {
        if (Object) {
            Object->ProcessDelegate(FunctionName, params, result);
        }
    }
};

// Modeled legacy 24-byte layout: an inline binding and a multicast-list pointer.
// Do not use this type for current 32-byte engine delegate slots.
struct FMultiScriptDelegate {
    FScriptDelegate SingleDelegate;                        // 0x00 (16 bytes)
    TArray<FScriptDelegate>* MulticastDelegate = nullptr;  // 0x10 - null unless multicast

    [[nodiscard]] bool IsBound() const noexcept {
        return SingleDelegate.IsBound() || (MulticastDelegate != nullptr && MulticastDelegate->Num() > 0);
    }
    void BindUFunction(UObject* object, FName functionName) noexcept {
        SingleDelegate.BindUFunction(object, functionName);
    }
    void Clear() noexcept {
        SingleDelegate.Clear();
        MulticastDelegate = nullptr;  // the array is engine-owned, just drop our view of it
    }
    // Invoke every bound handler: the multicast list when present, otherwise the single binding.
    void Broadcast(void* params, void* result = nullptr) const {
        if (MulticastDelegate != nullptr && MulticastDelegate->Num() > 0) {
            for (const FScriptDelegate& delegate : *MulticastDelegate) {
                delegate.ProcessDelegate(params, result);
            }
        } else {
            SingleDelegate.ProcessDelegate(params, result);
        }
    }
};

static_assert(sizeof(FScriptDelegate) == 0x10);
static_assert(sizeof(FMultiScriptDelegate) == 0x18);

} // namespace Kuru::SDK
