# Kuru::Gui

Draw an overlay from a mod. See the [gui guide](../modding/guides/gui.md).

## Functions

`EnsureContext()`
Bind your ImGui calls to Kuru's overlay. Call it at the top of `OnRender`, before any ImGui call. After it,
use the normal Dear ImGui API (`ImGui::Begin`, `ImGui::Text`, and so on).
