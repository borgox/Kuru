# Drawing an overlay

A mod draws its own overlay window by overriding `OnRender` and using Dear ImGui. Call
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

`OnRender` runs for enabled mods whenever Kuru builds an ImGui frame, even if F2/F3 are closed.
Use a mod-owned flag if you want to conditionally draw a window. Use the bundled ImGui core API;
extra libraries such as ImPlot are not included. The template already links the matching ImGui sources.

`OnRender` runs on the window thread. Present renders a completed draw-data snapshot on the render
thread. Queue game-object work with `Kuru::Execute`; read the values to display in `OnUpdate`, synchronize
the copied state, and draw it here. See the [threading guide](threading.md).

For values you want to keep, back them with a CVar (see [commands and settings](commands-and-cvars.md)) so
they persist across restarts and appear in the Settings tab.

## Mouse input

The current host feeds Win32/raw mouse input into ImGui and also polls mouse buttons with
`GetAsyncKeyState`. Polling reads button state; it does not consume input. The host separately filters
captured window messages and attempts to hook Rocket League's `RLRawMouseInput` window procedure and
`GetRawInputData`. ImGui's `WantCaptureMouse` alone is not an engine input gate.

If setup reports raw capture unavailable, clicks may reach both the overlay and the game.
Availability after eject/reinject remains an open issue; see [installation notes](../../installing.md).

The window procedure also consumes keyboard messages while ImGui requests keyboard capture, for
example while editing a text field. Keep custom ImGui work inside `OnRender`.
