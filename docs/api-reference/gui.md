# Kuru::Gui

Draw an overlay from a mod. See the [gui guide](../modding/guides/gui.md).

## Functions

### `EnsureContext()`

Bind your ImGui calls to Kuru's overlay. Call it at the top of `OnRender`, before any ImGui call. After it,
use the normal Dear ImGui API (`ImGui::Begin`, `ImGui::Text`, and so on).

`OnRender` runs during window-thread frame building, even with the built-in menu and console closed.
Keep the template's ImGui version matched to the host. Queue game-object work with `Kuru::Execute`
and synchronize any data shared with `OnUpdate` or worker threads.

`EnsureContext` binds once per mod DLL. If the host lacks its ImGui entries it does nothing and can
retry later. The umbrella `Kuru.hpp` includes `imgui.h`, so even mods without custom windows need
those bundled headers available.
