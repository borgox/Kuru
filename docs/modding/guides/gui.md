# Drawing an overlay

A mod draws its own window in the F2 overlay by overriding `OnRender` and using Dear ImGui. Call
`Kuru::Gui::EnsureContext()` first so your ImGui calls target Kuru's overlay.

```cpp
void OnRender() override {
    Kuru::Gui::EnsureContext();
    ImGui::Begin("My Mod");
    ImGui::Text("Hello from My Mod");
    if (ImGui::Button("Do something")) {
        Kuru::Info("clicked");
    }
    ImGui::End();
}
```

`OnRender` runs every frame the overlay is open. Anything in the ImGui API works - sliders, checkboxes,
tables, plots. Kuru bundles Dear ImGui, and a GUI mod links it (the template already does).

`OnRender` runs on the render thread, so don't touch game objects from it directly. Read the state you want
to show in `OnUpdate` (the game thread), store it, and draw the stored value here - see
[threading](threading.md).

For values you want to keep, back them with a CVar (see [commands and settings](commands-and-cvars.md)) so
they persist across restarts and appear in the Settings tab.
