#pragma once

#include <cstddef>
#include <cstdint>

namespace Kuru::SDK {

struct FPointer {
    std::uintptr_t Dummy = 0;

    constexpr FPointer() = default;
    constexpr explicit FPointer(std::uintptr_t value) : Dummy(value) {}
    template <typename T>
    [[nodiscard]] T* As() const noexcept {
        return reinterpret_cast<T*>(Dummy);
    }
};

struct FQWord {
    std::int32_t A = 0;
    std::int32_t B = 0;

    constexpr FQWord() = default;
    constexpr explicit FQWord(std::uint64_t value)
        : A(static_cast<std::int32_t>(value & 0xFFFFFFFFULL)),
          B(static_cast<std::int32_t>((value >> 32) & 0xFFFFFFFFULL)) {}

    [[nodiscard]] constexpr std::uint64_t ToUInt64() const noexcept {
        return static_cast<std::uint32_t>(A) | (static_cast<std::uint64_t>(static_cast<std::uint32_t>(B)) << 32);
    }
};

struct FDouble {
    double Value = 0.0;

    constexpr FDouble() = default;
    constexpr explicit FDouble(double value) : Value(value) {}
    [[nodiscard]] constexpr operator double() const noexcept { return Value; }
};

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
