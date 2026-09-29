#pragma once

#include "Names.hpp"
#include "ObjectModel.hpp"

namespace Kuru::SDK {

struct FScriptDelegate {
    UObject* Object = nullptr;
    FName FunctionName;
    std::uint8_t RLPad_0010[0x08]{};

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

struct FMulticastScriptDelegate {
    TArray<FScriptDelegate> InvocationList;

    [[nodiscard]] std::int32_t Num() const noexcept { return InvocationList.Num(); }
    [[nodiscard]] bool IsBound() const noexcept { return Num() > 0; }
    void Add(const FScriptDelegate& delegate) { InvocationList.Add(delegate); }
    void Remove(const FScriptDelegate& delegate) {
        for (std::int32_t index = 0; index < InvocationList.Count; ++index) {
            if (InvocationList[index].Object == delegate.Object && InvocationList[index].FunctionName == delegate.FunctionName) {
                InvocationList.Remove(index);
                return;
            }
        }
    }
    void Clear() { InvocationList.Empty(); }
    void Broadcast(void* params, void* result = nullptr) const {
        for (const FScriptDelegate& delegate : InvocationList) {
            delegate.ProcessDelegate(params, result);
        }
    }
};

static_assert(sizeof(FScriptDelegate) == 0x18);
static_assert(sizeof(FMulticastScriptDelegate) == 0x10);

} // namespace Kuru::SDK
