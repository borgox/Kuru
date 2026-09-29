#pragma once

#include <Kuru/HookContext.hpp>

#include <cstdint>

/* The stable ABI surface the host shares with every mod. Mods do NOT link the host's SDK: the
 * host resolves game state + owns the single hook registry, and pushes this table of function
 * pointers into each mod at load (KuruInitHostApi). Mod-facing wrappers (Kuru/Hooks.hpp, later
 * Commands/Log/Execute) call through it. Keep this struct append-only and version-gated.
*/
namespace Kuru {
namespace SDK {
class UObject;
}

enum class HookTiming : std::uint32_t {
    Pre = 0,   // before the original function runs (may inspect/modify params via the context)
    Post = 1,  // after the original runs (may read the result)
};

/* C ABI callback. `user` is the mod-owned context (its std::function holder). Runs on the game
 * thread inside the mod's code, so the mod's own inlined SDK interprets the HookContext.
 */
using HookCallbackFn = void (*)(HookContext* context, void* user);

using HookHandle = std::uint64_t;

using CommandCallbackFn = void (*)(const char* args, void* user);

// Event bus (see Kuru/Events.hpp). A listener thunk receives the type-erased event payload (the mod casts it back to its concrete event type) and the mod-owned `user` holder.
using ListenerThunkFn = void (*)(void* eventPayload, void* user);

struct HostApi {
    std::uint32_t Version;
    HookHandle (*RegisterHook)(const char* functionFullName, HookTiming timing, HookDispatch dispatch,
                               HookCallbackFn cb, void* user);
    void (*RemoveHook)(HookHandle handle);
    void (*Log)(int level, const char* tag, const char* message);
    void (*RegisterCommand)(const char* name, const char* help, CommandCallbackFn fn, void* user);
    // Shared ImGui context so a mod's own ImGui code draws into the host overlay (see Kuru/Gui.hpp).
    void* (*GetImGuiContext)();
    void (*GetImGuiAllocators)(void** allocFn, void** freeFn, void** userData);
    // Thread bridge (see Kuru/Thread.hpp), run work on the game thread.
    void (*Execute)(void (*thunk)(void* user), void* user);
    void (*SetTimeout)(void (*thunk)(void* user), void* user, float seconds);
    bool (*IsGameThread)();
    // Persistent storage (see Kuru/Storage.hpp).
    const char* (*GetKuruDir)();
    const char* (*GetGameDir)();
    const char* (*GetModDataDir)(const char* modName);
    // Global variables / CVars (see Kuru/Variables.hpp).
    void (*RegisterCVar)(const char* name, const char* defaultValue, const char* description);
    const char* (*GetCVar)(const char* name);
    void (*SetCVar)(const char* name, const char* value);
    void (*SuppressOverrideWarnings)();
    // Event bus Module-defined events fan out through these, function-call events (Pre/PostFunctionCallEvent) are produced by the hook engine and route through RegisterHook instead.
    HookHandle (*RegisterListener)(const char* identifier, const char* listenerName, ListenerThunkFn thunk,
                                   void* user, const char* filter, int priority);
    void (*UnregisterListener)(HookHandle handle);
    void (*DispatchEvent)(const char* identifier, const char* filter, void* eventPayload);
};

constexpr std::uint32_t HostApiVersion = 2;

void SetHostApi(const HostApi* api) noexcept;
[[nodiscard]] const HostApi* GetHostApi() noexcept;

// This mod's name, used to tag its log lines (set by KURU_EXPORT_MOD).
void SetModTag(const char* tag) noexcept;
[[nodiscard]] const char* GetModTag() noexcept;

} // namespace Kuru
