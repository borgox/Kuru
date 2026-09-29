#pragma once

#include <functional>

/*
 * In-game ImGui overlay over D3D11. Install() hooks the swapchain Present + the window WndProc;
 * the overlay initializes lazily on the first Present (render thread). F2 = menu, F3 = console.
 */
namespace Kuru::Gui {

bool Install();
void Shutdown();

// The host calls this callback inside its ImGui frame so mods can draw (their OnRender()).
void SetModRenderer(std::function<void()> renderer);

// Shared ImGui context accessors handed to mods so their own ImGui code draws into this overlay.
void* HostGetImGuiContext();
void HostGetImGuiAllocators(void** allocFn, void** freeFn, void** userData);

} // namespace Kuru::Gui
