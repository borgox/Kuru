#include "Gui.hpp"

#include "Console.hpp"
#include "HookEngine.hpp"
#include "ModRegistry.hpp"
#include "Reflector.hpp"
#include "Scanner.hpp"
#include "Storage.hpp"
#include "ThreadBridge.hpp"
#include "Variables.hpp"

#include <Kuru/Mod.hpp>

#include <Core/Globals.hpp>
#include <Core/ObjectModel.hpp>
#include <Core/Script.hpp>
#include <Engine/World.hpp>

#include "Hook.hpp"
#include <d3d11.h>
#include <dxgi.h>
#include <windows.h>

#include <imgui.h>
#include <backends/imgui_impl_dx11.h>
#include <backends/imgui_impl_win32.h>

#include <atomic>
#include <filesystem>
#include <mutex>
#include <string>
#include <vector>

// Provided by imgui_impl_win32.cpp.
extern IMGUI_IMPL_API LRESULT ImGui_ImplWin32_WndProcHandler(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);

namespace Kuru::Gui {
namespace {

void Log(const std::string& message) {
    OutputDebugStringA(("[Kuru] " + message + "\n").c_str());
    Kuru::Console::AddLog(message.c_str());
}

using PresentFn = HRESULT(__stdcall*)(IDXGISwapChain*, UINT, UINT);
using ResizeBuffersFn = HRESULT(__stdcall*)(IDXGISwapChain*, UINT, UINT, UINT, DXGI_FORMAT, UINT);

PresentFn gOriginalPresent = nullptr;
ResizeBuffersFn gOriginalResizeBuffers = nullptr;
WNDPROC gOriginalWndProc = nullptr;

void* gPresentTarget = nullptr;
void* gResizeTarget = nullptr;
Kuru::THook gPresentHook;
Kuru::THook gResizeHook;

ID3D11Device* gDevice = nullptr;
ID3D11DeviceContext* gContext = nullptr;
ID3D11RenderTargetView* gRenderTarget = nullptr;
HWND gWindow = nullptr;

std::atomic<bool> gInitialized{false};  // flipped true (release) only once the overlay is fully built
std::atomic<bool> gMenuOpen{false};     // toggled from the window thread (WndProc), read on the game thread
std::atomic<bool> gConsoleOpen{false};
std::function<void()> gModRenderer;
std::string gIniPath;

// The game thread builds the ImGui frame and hands the finished draw data to the render thread through this double buffer.
struct DrawListCopy {
    ImVector<ImDrawCmd> Cmd;
    ImVector<ImDrawIdx> Idx;
    ImVector<ImDrawVert> Vtx;
    ImDrawListFlags Flags = 0;
};
struct FrameSnapshot {
    bool Valid = false;
    ImVec2 DisplayPos{};
    ImVec2 DisplaySize{};
    ImVec2 FramebufferScale{1.0f, 1.0f};
    int TotalVtx = 0;
    int TotalIdx = 0;
    ImVector<ImTextureData*>* Textures = nullptr;  // points at the context's platform texture list
    std::vector<DrawListCopy> Lists;
};

std::mutex gSnapshotMutex;
FrameSnapshot gSnapshotFront;             // guarded by gSnapshotMutex: the latest frame for the render thread
FrameSnapshot gSnapshotBuild;             // window-thread scratch, swapped into the front once built
std::vector<ImDrawList*> gPresentLists;   // render-thread pool of draw lists that carry the snapshot buffers

// The frame is built on the window thread
constexpr UINT WM_KURU_BUILD = WM_APP + 0x4B55;  // 'KU'
std::atomic<bool> gBuildPending{false};

// TEMP diagnostics: how many raw input messages actually reach our WndProc.
std::atomic<int> gDiagLbDown{0};
std::atomic<int> gDiagMouseMove{0};
std::atomic<int> gDiagWmInput{0};

void BuildOverlayFrame();  // defined below; runs on the window thread

// The Kuru look: modern font, rounded translucent dark windows, rose-red accent.
void ApplyKuruStyle() {
    ImGuiIO& io = ImGui::GetIO();
    const std::filesystem::path fontPath =
        std::filesystem::path(L"C:\\Windows\\Fonts") / "segoeui.ttf";
    std::error_code ec;
    if (std::filesystem::exists(fontPath, ec)) {
        io.Fonts->AddFontFromFileTTF(fontPath.string().c_str(), 17.0f);
    }

    ImGuiStyle& s = ImGui::GetStyle();
    s.WindowRounding = 7.0f;
    s.ChildRounding = 6.0f;
    s.FrameRounding = 5.0f;
    s.PopupRounding = 5.0f;
    s.GrabRounding = 5.0f;
    s.TabRounding = 5.0f;
    s.ScrollbarRounding = 6.0f;
    s.WindowBorderSize = 1.0f;
    s.WindowPadding = ImVec2(12.0f, 12.0f);
    s.FramePadding = ImVec2(8.0f, 5.0f);
    s.ItemSpacing = ImVec2(9.0f, 7.0f);
    s.ScrollbarSize = 12.0f;
    s.GrabMinSize = 10.0f;
    s.WindowTitleAlign = ImVec2(0.5f, 0.5f);

    const ImVec4 accent = ImVec4(0.88f, 0.20f, 0.40f, 1.00f);    // rose-red
    const ImVec4 accentHi = ImVec4(0.96f, 0.30f, 0.50f, 1.00f);  // hover/active (lighter)
    const ImVec4 accentLo = ImVec4(0.72f, 0.13f, 0.31f, 1.00f);  // pressed/inactive (darker)
    auto fade = [](ImVec4 col, float a) { return ImVec4(col.x, col.y, col.z, a); };

    ImVec4* c = s.Colors;
    c[ImGuiCol_WindowBg] = ImVec4(0.09f, 0.09f, 0.10f, 0.96f);
    c[ImGuiCol_ChildBg] = ImVec4(0.11f, 0.11f, 0.12f, 1.00f);
    c[ImGuiCol_PopupBg] = ImVec4(0.10f, 0.10f, 0.11f, 0.98f);
    c[ImGuiCol_Border] = ImVec4(0.24f, 0.22f, 0.24f, 0.55f);
    c[ImGuiCol_FrameBg] = ImVec4(0.16f, 0.16f, 0.18f, 1.00f);
    c[ImGuiCol_FrameBgHovered] = ImVec4(0.24f, 0.20f, 0.23f, 1.00f);
    c[ImGuiCol_FrameBgActive] = ImVec4(0.30f, 0.19f, 0.24f, 1.00f);
    c[ImGuiCol_TitleBg] = ImVec4(0.10f, 0.10f, 0.11f, 1.00f);
    c[ImGuiCol_TitleBgActive] = accentLo;
    c[ImGuiCol_CheckMark] = accentHi;
    c[ImGuiCol_SliderGrab] = accent;
    c[ImGuiCol_SliderGrabActive] = accentHi;
    c[ImGuiCol_Button] = fade(accent, 0.55f);
    c[ImGuiCol_ButtonHovered] = fade(accentHi, 0.85f);
    c[ImGuiCol_ButtonActive] = accentLo;
    c[ImGuiCol_Header] = fade(accent, 0.50f);
    c[ImGuiCol_HeaderHovered] = fade(accentHi, 0.70f);
    c[ImGuiCol_HeaderActive] = accent;
    c[ImGuiCol_SeparatorHovered] = accent;
    c[ImGuiCol_SeparatorActive] = accentHi;
    c[ImGuiCol_ResizeGrip] = fade(accent, 0.30f);
    c[ImGuiCol_ResizeGripHovered] = fade(accentHi, 0.65f);
    c[ImGuiCol_ResizeGripActive] = accentHi;
    c[ImGuiCol_Tab] = ImVec4(0.16f, 0.14f, 0.15f, 1.00f);
    c[ImGuiCol_TabHovered] = fade(accentHi, 0.80f);
    c[ImGuiCol_TabActive] = accent;
    c[ImGuiCol_TextSelectedBg] = fade(accent, 0.40f);
    c[ImGuiCol_ScrollbarGrabActive] = accent;
}

void CreateRenderTarget(IDXGISwapChain* swapChain) {
    ID3D11Texture2D* backBuffer = nullptr;
    if (SUCCEEDED(swapChain->GetBuffer(0, IID_PPV_ARGS(&backBuffer))) && backBuffer) {
        gDevice->CreateRenderTargetView(backBuffer, nullptr, &gRenderTarget);
        backBuffer->Release();
    }
}

// Structural/window-management messages that are not input and must always reach the game untouched.
bool IsMessageCritical(UINT msg) {
    switch (msg) {
    case WM_DESTROY:
    case WM_NCDESTROY:
    case WM_CLOSE:
    case WM_QUIT:
    case WM_ACTIVATE:
    case WM_ACTIVATEAPP:
    case WM_NCACTIVATE:
    case WM_SIZE:
    case WM_SIZING:
    case WM_MOVE:
    case WM_MOVING:
    case WM_WINDOWPOSCHANGED:
    case WM_WINDOWPOSCHANGING:
    case WM_DISPLAYCHANGE:
        return true;
    default:
        return false;
    }
}

void FeedRawMouseButtons(LPARAM lParam) {
    RAWINPUT raw{};
    UINT size = sizeof(raw);
    if (GetRawInputData(reinterpret_cast<HRAWINPUT>(lParam), RID_INPUT, &raw, &size,
                        sizeof(RAWINPUTHEADER)) == static_cast<UINT>(-1) ||
        raw.header.dwType != RIM_TYPEMOUSE) {
        return;
    }
    const RAWMOUSE& m = raw.data.mouse;
    ImGuiIO& io = ImGui::GetIO();
    const USHORT flags = m.usButtonFlags;
    if (flags & RI_MOUSE_LEFT_BUTTON_DOWN)   io.AddMouseButtonEvent(0, true);
    if (flags & RI_MOUSE_LEFT_BUTTON_UP)     io.AddMouseButtonEvent(0, false);
    if (flags & RI_MOUSE_RIGHT_BUTTON_DOWN)  io.AddMouseButtonEvent(1, true);
    if (flags & RI_MOUSE_RIGHT_BUTTON_UP)    io.AddMouseButtonEvent(1, false);
    if (flags & RI_MOUSE_MIDDLE_BUTTON_DOWN) io.AddMouseButtonEvent(2, true);
    if (flags & RI_MOUSE_MIDDLE_BUTTON_UP)   io.AddMouseButtonEvent(2, false);
    if (flags & RI_MOUSE_WHEEL) {
        io.AddMouseWheelEvent(0.0f, static_cast<float>(static_cast<SHORT>(m.usButtonData)) /
                                        static_cast<float>(WHEEL_DELTA));
    }
}

DWORD RawInputDeviceType(LPARAM lParam) {
    RAWINPUTHEADER header{};
    UINT size = sizeof(header);
    if (GetRawInputData(reinterpret_cast<HRAWINPUT>(lParam), RID_HEADER, &header, &size,
                        sizeof(RAWINPUTHEADER)) == static_cast<UINT>(-1)) {
        return static_cast<DWORD>(-1);
    }
    return header.dwType;
}

bool IsMouseInput(UINT msg, LPARAM lParam) {
    if (msg >= WM_MOUSEFIRST && msg <= WM_MOUSELAST) {
        return true;
    }
    return msg == WM_INPUT && RawInputDeviceType(lParam) == RIM_TYPEMOUSE;
}

bool IsKeyboardInput(UINT msg) {
    return msg >= WM_KEYFIRST && msg <= WM_KEYLAST;
}

void SetGameHardwareCursor(bool visible) {
    using namespace Kuru::SDK;
    static UFunction* getEngineFn = UObject::FindFunction("Engine.Engine.GetEngine");
    static UFunction* setFn = [] {
        UFunction* f = UObject::FindFunction("ProjectX.GameViewportClient_X.SetHardwareMouseCursorVisibility");
        return f ? f : UObject::FindFunction("Engine.GameViewportClient.SetHardwareMouseCursorVisibility");
    }();
    if (!getEngineFn || !getEngineFn->Func || !setFn) {
        return;
    }
    std::uint8_t endParms[8] = {};
    FStateFrame frame{};
    frame.Frame.Node = reinterpret_cast<UStruct*>(getEngineFn);
    frame.Frame.Object = getEngineFn;
    frame.Frame.Code = endParms;
    UEngine* engine = nullptr;
    getEngineFn->Func(getEngineFn, frame, &engine);
    if (!engine || !engine->GameViewport) {
        return;
    }
    struct Params {
        std::uint32_t bIsVisible;
    } params{visible ? 1u : 0u};
    engine->GameViewport->ProcessEvent(setFn, &params);
}

// On close, let GFxEngine recompute the correct cursor state
void RestoreGameCursor() {
    using namespace Kuru::SDK;
    static UClass* gfxClass = UObject::FindClass("ProjectX.GFxEngine_X");
    static UFunction* updateFn =
        UObject::FindFunction("ProjectX.GFxEngine_X.UpdateHardwareMouseCursorVisibility");
    if (!gfxClass || !updateFn) {
        return;
    }
    auto* objects = GetGlobalObjects();
    if (!objects) {
        return;
    }
    for (UObject* object : *objects) {
        if (object && object->IsA(gfxClass) && !object->IsTemplate()) {
            object->ProcessEvent(updateFn, nullptr);
            return;
        }
    }
}

LRESULT __stdcall DetourWndProc(HWND window, UINT msg, WPARAM wParam, LPARAM lParam) {
    // Our own frame-build request, posted by the Present detour.
    if (msg == WM_KURU_BUILD) {
        gBuildPending.store(false, std::memory_order_relaxed);
        BuildOverlayFrame();
        return 0;
    }
    if (msg == WM_LBUTTONDOWN) gDiagLbDown.fetch_add(1, std::memory_order_relaxed);
    else if (msg == WM_MOUSEMOVE) gDiagMouseMove.fetch_add(1, std::memory_order_relaxed);
    else if (msg == WM_INPUT) gDiagWmInput.fetch_add(1, std::memory_order_relaxed);
    // Non-input/structural messages, and anything before the overlay is ready, go straight to the game.
    if (IsMessageCritical(msg) || !gInitialized.load(std::memory_order_acquire)) {
        return CallWindowProcW(gOriginalWndProc, window, msg, wParam, lParam);
    }

    const bool uiOpen = gMenuOpen.load(std::memory_order_relaxed) ||
                        gConsoleOpen.load(std::memory_order_relaxed);

    // Let ImGui set the cursor (show the arrow) while a Kuru window is open
    if (msg == WM_SETCURSOR) {
        if (uiOpen) {
            return ImGui_ImplWin32_WndProcHandler(window, msg, wParam, lParam);
        }
        return CallWindowProcW(gOriginalWndProc, window, msg, wParam, lParam);
    }

    ImGui_ImplWin32_WndProcHandler(window, msg, wParam, lParam);

    // RL delivers the mouse only as raw input, so feed ImGui the button/wheel events ourselves.
    if (msg == WM_INPUT) {
        FeedRawMouseButtons(lParam);
    }

    // F2/F3 toggle the menu/console. Bit 30 of lParam filters out auto-repeat. Consume them.
    if (msg == WM_KEYDOWN && (HIWORD(lParam) & KF_REPEAT) == 0) {
        if (wParam == VK_F2) {
            gMenuOpen.store(!gMenuOpen.load(std::memory_order_relaxed), std::memory_order_relaxed);
            return 0;
        }
        if (wParam == VK_F3) {
            gConsoleOpen.store(!gConsoleOpen.load(std::memory_order_relaxed), std::memory_order_relaxed);
            return 0;
        }
    }

    // Keep from the game only the input ImGui is actually using
    const ImGuiIO& io = ImGui::GetIO();
    if (io.WantCaptureMouse && IsMouseInput(msg, lParam)) {
        return DefWindowProcW(window, msg, wParam, lParam);
    }
    if (io.WantCaptureKeyboard && IsKeyboardInput(msg)) {
        return DefWindowProcW(window, msg, wParam, lParam);
    }

    return CallWindowProcW(gOriginalWndProc, window, msg, wParam, lParam);
}

void InitializeOverlay(IDXGISwapChain* swapChain) {
    if (FAILED(swapChain->GetDevice(IID_PPV_ARGS(&gDevice)))) {
        return;
    }
    gDevice->GetImmediateContext(&gContext);

    DXGI_SWAP_CHAIN_DESC desc{};
    swapChain->GetDesc(&desc);
    gWindow = desc.OutputWindow;

    CreateRenderTarget(swapChain);

    ImGui::CreateContext();
    // Persist window positions/sizes across sessions in %APPDATA%\Kuru\imgui.ini
    gIniPath = (Kuru::Storage::KuruDir() / "imgui.ini").string();
    ImGui::GetIO().IniFilename = gIniPath.empty() ? nullptr : gIniPath.c_str();
    ImGui::StyleColorsDark();
    ApplyKuruStyle();  // Kuru font + rose-red theme; every mod window inherits it
    ImGui_ImplWin32_Init(gWindow);
    ImGui_ImplDX11_Init(gDevice, gContext);

    gOriginalWndProc = reinterpret_cast<WNDPROC>(
        SetWindowLongPtrW(gWindow, GWLP_WNDPROC, reinterpret_cast<LONG_PTR>(&DetourWndProc)));

    gInitialized.store(true, std::memory_order_release);
    Log("overlay initialized (press F2 to toggle the Kuru menu)");
}

// Deep-copy the finished draw data into the build buffer, then swap it in as the new front snapshot.
void CaptureDrawData(ImDrawData* dd) {
    if (!dd) {
        return;
    }
    FrameSnapshot& s = gSnapshotBuild;
    s.Valid = dd->Valid && dd->CmdLists.Size > 0;
    s.DisplayPos = dd->DisplayPos;
    s.DisplaySize = dd->DisplaySize;
    s.FramebufferScale = dd->FramebufferScale;
    s.TotalVtx = dd->TotalVtxCount;
    s.TotalIdx = dd->TotalIdxCount;
    s.Textures = dd->Textures;  // pointer into the context
    const int listCount = dd->CmdLists.Size;
    s.Lists.resize(static_cast<std::size_t>(listCount));
    for (int i = 0; i < listCount; ++i) {
        const ImDrawList* src = dd->CmdLists[i];
        s.Lists[i].Cmd = src->CmdBuffer;  // ImVector::operator= reuses the existing capacity
        s.Lists[i].Idx = src->IdxBuffer;
        s.Lists[i].Vtx = src->VtxBuffer;
        s.Lists[i].Flags = src->Flags;
    }
    std::scoped_lock lock(gSnapshotMutex);
    std::swap(gSnapshotFront, gSnapshotBuild);
}

// Builds the entire ImGui frame on the game thread. Registered as the ThreadBridge per-frame builder.
void BuildOverlayFrame() {
    if (!gInitialized.load(std::memory_order_acquire)) {
        return;
    }
    static bool lastUiOpen = false;
    const bool uiOpen = gMenuOpen.load(std::memory_order_relaxed) ||
                        gConsoleOpen.load(std::memory_order_relaxed);
    if (uiOpen) {
        SetGameHardwareCursor(true);
    } else if (lastUiOpen) {
        RestoreGameCursor();
    }
    lastUiOpen = uiOpen;

    ImGui_ImplDX11_NewFrame();
    ImGui_ImplWin32_NewFrame();
    {
        ImGuiIO& pio = ImGui::GetIO();
        pio.AddMouseButtonEvent(0, (GetAsyncKeyState(VK_LBUTTON) & 0x8000) != 0);
        pio.AddMouseButtonEvent(1, (GetAsyncKeyState(VK_RBUTTON) & 0x8000) != 0);
        pio.AddMouseButtonEvent(2, (GetAsyncKeyState(VK_MBUTTON) & 0x8000) != 0);
    }
    ImGui::NewFrame();

    // Always-on indicator so the overlay is visibly alive even before opening the menu.
    ImGui::SetNextWindowPos(ImVec2(12, 12), ImGuiCond_Always);
    ImGui::SetNextWindowBgAlpha(0.35f);
    ImGui::Begin("##kuru_status", nullptr,
                 ImGuiWindowFlags_NoDecoration | ImGuiWindowFlags_AlwaysAutoResize |
                 ImGuiWindowFlags_NoInputs | ImGuiWindowFlags_NoNav);
    ImGui::TextColored(ImVec4(0.96f, 0.30f, 0.50f, 1.00f), "Kuru");
    ImGui::SameLine();
    ImGui::TextDisabled("F2 menu  |  F3 console");
    ImGui::End();

    bool menuOpen = gMenuOpen.load(std::memory_order_relaxed);
    if (menuOpen) {
        ImGui::SetNextWindowPos(ImVec2(60, 60), ImGuiCond_FirstUseEver);
        ImGui::SetNextWindowSize(ImVec2(520, 360), ImGuiCond_FirstUseEver);
        ImGui::Begin("Kuru", &menuOpen);
        if (ImGui::BeginTabBar("kuru_tabs")) {
            if (ImGui::BeginTabItem("Mods")) {
                auto& mods = Kuru::ModRegistry::Entries();
                ImGui::Text("%d mod(s) loaded", static_cast<int>(mods.size()));
                ImGui::Separator();
                for (Kuru::ModRegistry::Entry& mod : mods) {
                    ImGui::PushID(&mod);
                    bool enabled = mod.enabled;
                    if (ImGui::Checkbox("##enabled", &enabled)) {
                        // Enable/disable runs the mod's OnLoad/OnUnload + hook cleanup. Done on the game thread
                        Kuru::ModRegistry::Entry* entry = &mod;
                        Kuru::ThreadBridge::EnqueueGameThread(
                            [entry, enabled] { Kuru::ModRegistry::SetEnabled(*entry, enabled); });
                    }
                    ImGui::SameLine();
                    ImGui::Text("%s  v%s", mod.name.c_str(), mod.version.c_str());
                    if (!mod.author.empty()) {
                        ImGui::SameLine();
                        ImGui::TextDisabled("by %s", mod.author.c_str());
                    }
                    ImGui::SameLine();
                    if (ImGui::SmallButton("Reload")) {
                        Kuru::ModRegistry::Entry* entry = &mod;
                        Kuru::ThreadBridge::EnqueueGameThread(
                            [entry] { Kuru::ModRegistry::Reload(*entry); });
                    }
                    if (ImGui::IsItemHovered()) {
                        ImGui::SetTooltip("destroy and recreate the mod (resets its state)");
                    }
                    ImGui::PopID();
                }
                ImGui::EndTabItem();
            }
            if (ImGui::BeginTabItem("Functions")) {
                Kuru::Scanner::Draw();
                ImGui::EndTabItem();
            }
            if (ImGui::BeginTabItem("Objects")) {
                Kuru::Reflector::Draw();
                ImGui::EndTabItem();
            }
            if (ImGui::BeginTabItem("Settings")) {
                Kuru::Variables::Draw();
                ImGui::EndTabItem();
            }
            ImGui::EndTabBar();
        }
        ImGui::End();
    }
    gMenuOpen.store(menuOpen, std::memory_order_relaxed);  // the window's close box can clear it

    bool consoleOpen = gConsoleOpen.load(std::memory_order_relaxed);
    if (consoleOpen) {
        Kuru::Console::Draw(&consoleOpen);
    }
    gConsoleOpen.store(consoleOpen, std::memory_order_relaxed);

    // Let mods draw into the same frame/context. Isolate so a throwing mod can't break the frame.
    if (gModRenderer) {
        try {
            gModRenderer();
        } catch (...) {
        }
    }

    ImGui::Render();
    CaptureDrawData(ImGui::GetDrawData());
}

// Presents the latest snapshot. Runs on the render thread, which only uploads and draws, never builds.
void RenderSnapshot() {
    if (!gRenderTarget) {
        return;
    }
    std::scoped_lock lock(gSnapshotMutex);
    FrameSnapshot& s = gSnapshotFront;
    if (!s.Valid || s.Lists.empty()) {
        return;
    }
    while (gPresentLists.size() < s.Lists.size()) {
        gPresentLists.push_back(IM_NEW(ImDrawList)(ImGui::GetDrawListSharedData()));
    }

    const int listCount = static_cast<int>(s.Lists.size());
    ImDrawData drawData;
    drawData.Valid = true;
    drawData.DisplayPos = s.DisplayPos;
    drawData.DisplaySize = s.DisplaySize;
    drawData.FramebufferScale = s.FramebufferScale;
    drawData.TotalVtxCount = s.TotalVtx;
    drawData.TotalIdxCount = s.TotalIdx;
    drawData.Textures = s.Textures;
    // Lend the snapshot's buffers to the pooled draw lists, render, then take them back.
    for (int i = 0; i < listCount; ++i) {
        ImDrawList* list = gPresentLists[i];
        list->CmdBuffer.swap(s.Lists[i].Cmd);
        list->IdxBuffer.swap(s.Lists[i].Idx);
        list->VtxBuffer.swap(s.Lists[i].Vtx);
        list->Flags = s.Lists[i].Flags;
        drawData.CmdLists.push_back(list);
    }

    gContext->OMSetRenderTargets(1, &gRenderTarget, nullptr);
    ImGui_ImplDX11_RenderDrawData(&drawData);

    for (int i = 0; i < listCount; ++i) {
        gPresentLists[i]->CmdBuffer.swap(s.Lists[i].Cmd);
        gPresentLists[i]->IdxBuffer.swap(s.Lists[i].Idx);
        gPresentLists[i]->VtxBuffer.swap(s.Lists[i].Vtx);
    }
}

HRESULT __stdcall DetourPresent(IDXGISwapChain* swapChain, UINT syncInterval, UINT flags) {
    Kuru::ThreadBridge::NotifyPresent();  // drives the game-thread tick (consumed in the PE detour)
    if (!gInitialized.load(std::memory_order_acquire)) {
        InitializeOverlay(swapChain);
    }
    if (gInitialized.load(std::memory_order_acquire)) {
        if (!gRenderTarget) {
            CreateRenderTarget(swapChain);
        }
        // Ask the window thread to build the next frame, then present the latest finished one.
        if (gWindow && !gBuildPending.exchange(true, std::memory_order_relaxed)) {
            PostMessageW(gWindow, WM_KURU_BUILD, 0, 0);
        }
        RenderSnapshot();  // render thread only presents, the window thread builds
    }
    return gOriginalPresent(swapChain, syncInterval, flags);
}

HRESULT __stdcall DetourResizeBuffers(IDXGISwapChain* swapChain, UINT bufferCount, UINT width,
                                      UINT height, DXGI_FORMAT format, UINT flags) {
    if (gRenderTarget) {
        gRenderTarget->Release();
        gRenderTarget = nullptr;
    }
    return gOriginalResizeBuffers(swapChain, bufferCount, width, height, format, flags);
}

/*
 * Spin up a throwaway device/swapchain just to read the IDXGISwapChain vtable, so we can hook the real game's Present/ResizeBuffers by address.
*/
bool ResolveSwapChainFunctions() {
    WNDCLASSEXW wc{};
    wc.cbSize = sizeof(wc);
    wc.lpfnWndProc = DefWindowProcW;
    wc.hInstance = GetModuleHandleW(nullptr);
    wc.lpszClassName = L"KuruDummyWindow";
    RegisterClassExW(&wc);
    HWND dummy = CreateWindowW(wc.lpszClassName, L"", WS_OVERLAPPEDWINDOW, 0, 0, 100, 100,
                               nullptr, nullptr, wc.hInstance, nullptr);

    DXGI_SWAP_CHAIN_DESC sd{};
    sd.BufferCount = 1;
    sd.BufferDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
    sd.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
    sd.OutputWindow = dummy;
    sd.SampleDesc.Count = 1;
    sd.Windowed = TRUE;
    sd.SwapEffect = DXGI_SWAP_EFFECT_DISCARD;

    const D3D_FEATURE_LEVEL levels[] = {D3D_FEATURE_LEVEL_11_0};
    D3D_FEATURE_LEVEL obtained{};
    IDXGISwapChain* swapChain = nullptr;
    ID3D11Device* device = nullptr;
    ID3D11DeviceContext* context = nullptr;

    const HRESULT hr = D3D11CreateDeviceAndSwapChain(
        nullptr, D3D_DRIVER_TYPE_HARDWARE, nullptr, 0, levels, 1, D3D11_SDK_VERSION, &sd,
        &swapChain, &device, &obtained, &context);

    bool ok = false;
    if (SUCCEEDED(hr) && swapChain) {
        void** vtable = *reinterpret_cast<void***>(swapChain);
        gPresentTarget = vtable[8];        // IDXGISwapChain::Present
        gResizeTarget = vtable[13];        // IDXGISwapChain::ResizeBuffers
        ok = true;
    }
    if (swapChain) swapChain->Release();
    if (context) context->Release();
    if (device) device->Release();
    DestroyWindow(dummy);
    UnregisterClassW(wc.lpszClassName, wc.hInstance);
    return ok;
}

} // namespace

bool Install() {
    if (!ResolveSwapChainFunctions() || !gPresentTarget) {
        Log("GUI install failed: could not resolve swapchain vtable");
        return false;
    }
    if (!gPresentHook.Install(gPresentTarget, reinterpret_cast<void*>(&DetourPresent),
                              reinterpret_cast<void**>(&gOriginalPresent))) {
        Log("GUI install failed: Present hook");
        return false;
    }
    (void)gResizeHook.Install(gResizeTarget, reinterpret_cast<void*>(&DetourResizeBuffers),
                              reinterpret_cast<void**>(&gOriginalResizeBuffers));  // optional, ok if null
    Log("GUI installed");
    return true;
}

void SetModRenderer(std::function<void()> renderer) {
    gModRenderer = std::move(renderer);
}

void* HostGetImGuiContext() {
    return ImGui::GetCurrentContext();
}

void HostGetImGuiAllocators(void** allocFn, void** freeFn, void** userData) {
    ImGui::GetAllocatorFunctions(reinterpret_cast<ImGuiMemAllocFunc*>(allocFn),
                                 reinterpret_cast<ImGuiMemFreeFunc*>(freeFn), userData);
}

void Shutdown() {
    // Clearing this makes BuildOverlayFrame  early-return, so no new frames are built.
    const bool wasInitialized = gInitialized.exchange(false, std::memory_order_acq_rel);
    gPresentHook.Remove();
    gResizeHook.Remove();
    if (wasInitialized) {
        if (gWindow && gOriginalWndProc) {
            SetWindowLongPtrW(gWindow, GWLP_WNDPROC, reinterpret_cast<LONG_PTR>(gOriginalWndProc));
        }
        ImGui_ImplDX11_Shutdown();
        ImGui_ImplWin32_Shutdown();
        ImGui::DestroyContext();
    }
    {
        std::scoped_lock lock(gSnapshotMutex);
        for (ImDrawList* list : gPresentLists) {
            IM_DELETE(list);
        }
        gPresentLists.clear();
        gSnapshotFront = FrameSnapshot{};
        gSnapshotBuild = FrameSnapshot{};
    }
    if (gRenderTarget) gRenderTarget->Release();
    if (gContext) gContext->Release();
    if (gDevice) gDevice->Release();
}

} // namespace Kuru::Gui
