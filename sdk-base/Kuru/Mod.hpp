#pragma once

#include <Core/Runtime.hpp>
#include <Kuru/HostApi.hpp>

#include <cstdint>

#if defined(_WIN32)
#define KURU_SDK_EXPORT extern "C" __declspec(dllexport)
#else
#define KURU_SDK_EXPORT extern "C" __attribute__((visibility("default")))
#endif

namespace Kuru {

class IMod {
public:
    virtual ~IMod() = default;
    virtual bool OnLoad() { return true; }
    virtual void OnUnload() {}
    virtual void OnUpdate(float deltaSeconds) { (void)deltaSeconds; }
    // Draw the mod's own ImGui windows/tabs. Called inside the host overlay's frame, use Kuru::Gui::EnsureContext() first (see Kuru/Gui.hpp).
    virtual void OnRender() {}
};

struct ModInfo {
    const char* Name = "";
    const char* Author = "";
    const char* Version = "";
};

using CreateModFunc = IMod* (*)();
using DestroyModFunc = void (*)(IMod*);

} // namespace Kuru

#define KURU_EXPORT_MOD(ModType, ModName, ModAuthor, ModVersion) \
    KURU_SDK_EXPORT const Kuru::ModInfo* KuruGetModInfo() { \
        static constexpr Kuru::ModInfo info{ModName, ModAuthor, ModVersion}; \
        return &info; \
    } \
    KURU_SDK_EXPORT Kuru::IMod* KuruCreateMod() { \
        return new ModType(); \
    } \
    KURU_SDK_EXPORT void KuruDestroyMod(Kuru::IMod* mod) { \
        delete mod; \
    } \
    /* The HostApi version this mod was BUILT against. The loader refuses a mod that needs a newer */ \
    /* host than it provides (forward-incompatible), so a mismatched download is skipped. */ \
    KURU_SDK_EXPORT std::uint32_t KuruRequiredApiVersion() { \
        return Kuru::HostApiVersion; \
    } \
    /* The loader calls this before OnLoad to populate THIS mod's own SDK globals with the */ \
    /* resolved game bindings, so the mod's inlined SDK code sees live GObjects/GNames/etc. */ \
    KURU_SDK_EXPORT void KuruInitRuntime(const Kuru::SDK::RuntimeBindings* bindings) { \
        if (bindings) { \
            Kuru::SDK::SetRuntimeBindings(*bindings); \
        } \
    } \
    /* The loader pushes its API table (hook registration, later commands/log) into this mod. */ \
    KURU_SDK_EXPORT void KuruInitHostApi(const Kuru::HostApi* api) { \
        Kuru::SetModTag(ModName); \
        Kuru::SetHostApi(api); \
    }
