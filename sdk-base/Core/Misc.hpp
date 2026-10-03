#pragma once

#include <cstddef>
#include <cstdint>

namespace Kuru::SDK {

// Aliases not wrappers
using FPointer = void*;
using FQWord = std::uint64_t;
using FDouble = double;

class UObject;

struct FScriptInterface {
    UObject* ObjectPointer = nullptr;
    void* InterfacePointer = nullptr;

    [[nodiscard]] UObject* GetObject() const noexcept { return ObjectPointer; }
    [[nodiscard]] void* GetInterface() const noexcept { return InterfacePointer; }
    void SetObject(UObject* object) noexcept { ObjectPointer = object; }
    void SetInterface(void* interfacePointer) noexcept { InterfacePointer = interfacePointer; }
};

template <typename InterfaceType>
struct TScriptInterface : public FScriptInterface {
    [[nodiscard]] InterfaceType* operator->() const noexcept {
        return reinterpret_cast<InterfaceType*>(InterfacePointer);
    }

    [[nodiscard]] InterfaceType* GetInterface() const noexcept {
        return reinterpret_cast<InterfaceType*>(InterfacePointer);
    }

    [[nodiscard]] explicit operator bool() const noexcept { return ObjectPointer != nullptr && InterfacePointer != nullptr; }
};

static_assert(sizeof(FPointer) == 0x08);
static_assert(sizeof(FQWord) == 0x08);
static_assert(sizeof(FDouble) == 0x08);
static_assert(sizeof(FScriptInterface) == 0x10);

} // namespace Kuru::SDK
