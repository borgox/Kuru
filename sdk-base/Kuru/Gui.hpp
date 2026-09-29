#pragma once

#include <Kuru/HostApi.hpp>

#include <imgui.h>

/* Mod-facing GUI helper. A mod that wants to draw its own windows/tabs links ImGui, includes this
 * and calls EnsureContext() at the top of IMod::OnRender(), that binds the mod's ImGui to the
 * host's shared context/allocator so its ImGui::Begin/End render into the Kuru overlay.
*/
namespace Kuru::Gui {

inline void EnsureContext() {
    static bool done = false;
    if (done) {
        return;
    }
    const HostApi* api = GetHostApi();
    if (!api || !api->GetImGuiContext || !api->GetImGuiAllocators) {
        return;
    }
    ImGui::SetCurrentContext(static_cast<ImGuiContext*>(api->GetImGuiContext()));
    void* allocFn = nullptr;
    void* freeFn = nullptr;
    void* userData = nullptr;
    api->GetImGuiAllocators(&allocFn, &freeFn, &userData);
    ImGui::SetAllocatorFunctions(reinterpret_cast<ImGuiMemAllocFunc>(allocFn),
                                 reinterpret_cast<ImGuiMemFreeFunc>(freeFn), userData);
    done = true;
}

} // namespace Kuru::Gui
