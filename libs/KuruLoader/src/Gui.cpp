#include "Gui.hpp"

#include "Console.hpp"
#include "HookEngine.hpp"
#include "ModRegistry.hpp"
#include "Reflector.hpp"
#include "Scanner.hpp"
#include "ThreadBridge.hpp"
#include "Variables.hpp"

#include <Kuru/Mod.hpp>

#include <MinHook.h>
#include <d3d11.h>
#include <dxgi.h>
#include <windows.h>

#include <imgui.h>
#include <backends/imgui_impl_dx11.h>
#include <backends/imgui_impl_win32.h>

#include <cstdlib>
#include <string>

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

ID3D11Device* gDevice = nullptr;
ID3D11DeviceContext* gContext = nullptr;
ID3D11RenderTargetView* gRenderTarget = nullptr;
HWND gWindow = nullptr;

bool gInitialized = false;
bool gMenuOpen = false;
bool gConsoleOpen = false;
std::function<void()> gModRenderer;
std::string gIniPath;

// The Kuru look: modern font, rounded translucent dark windows, rose-red accent.
void ApplyKuruStyle() {
    ImGuiIO& io = ImGui::GetIO();
    const char* fontPath = "C:\\Windows\\Fonts\\segoeui.ttf";
    if (GetFileAttributesA(fontPath) != INVALID_FILE_ATTRIBUTES) {
        io.Fonts->AddFontFromFileTTF(fontPath, 17.0f);
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

LRESULT __stdcall DetourWndProc(HWND window, UINT msg, WPARAM wParam, LPARAM lParam) {
    // Always let ImGui observe messages so its input/focus state stays consistent
    if (gInitialized) {
        ImGui_ImplWin32_WndProcHandler(window, msg, wParam, lParam);
        if (gMenuOpen || gConsoleOpen) {
            const ImGuiIO& io = ImGui::GetIO();
            // Only ever swallow actual mouse/keyboard input the UI is using
            const bool isMouse = (msg >= WM_MOUSEFIRST && msg <= WM_MOUSELAST);
            const bool isKeyboard = (msg >= WM_KEYFIRST && msg <= WM_KEYLAST);
            if ((isMouse && io.WantCaptureMouse) || (isKeyboard && io.WantCaptureKeyboard)) {
                return true;
            }
        }
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
    if (const char* appData = std::getenv("APPDATA")) {
        const std::string dir = std::string(appData) + "\\Kuru";
        CreateDirectoryA(dir.c_str(), nullptr);
        gIniPath = dir + "\\imgui.ini";
        ImGui::GetIO().IniFilename = gIniPath.c_str();
    } else {
        ImGui::GetIO().IniFilename = nullptr;
    }
    ImGui::StyleColorsDark();
    ApplyKuruStyle();  // Kuru font + rose-red theme; every mod window inherits it
    ImGui_ImplWin32_Init(gWindow);
    ImGui_ImplDX11_Init(gDevice, gContext);

    gOriginalWndProc = reinterpret_cast<WNDPROC>(
        SetWindowLongPtrW(gWindow, GWLP_WNDPROC, reinterpret_cast<LONG_PTR>(&DetourWndProc)));

    gInitialized = true;
    Log("overlay initialized (press F2 to toggle the Kuru menu)");
}

void RenderOverlay() {
    if (!gRenderTarget) {
        return;
    }

    // Toggle via async key state
    // F2 = main menu, F3 = console.
    static bool wasF2 = false;
    const bool f2 = (GetAsyncKeyState(VK_F2) & 0x8000) != 0;
    if (f2 && !wasF2) {
        gMenuOpen = !gMenuOpen;
    }
    wasF2 = f2;

    static bool wasF3 = false;
    const bool f3 = (GetAsyncKeyState(VK_F3) & 0x8000) != 0;
    if (f3 && !wasF3) {
        gConsoleOpen = !gConsoleOpen;
    }
    wasF3 = f3;

    ImGui_ImplDX11_NewFrame();
    ImGui_ImplWin32_NewFrame();
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

    if (gMenuOpen) {
        ImGui::SetNextWindowPos(ImVec2(60, 60), ImGuiCond_FirstUseEver);
        ImGui::SetNextWindowSize(ImVec2(520, 360), ImGuiCond_FirstUseEver);
        ImGui::Begin("Kuru", &gMenuOpen);
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

    if (gConsoleOpen) {
        Kuru::Console::Draw(&gConsoleOpen);
    }

    // Let mods draw into the same frame/context. Isolate so a throwing mod can't break the frame.
    if (gModRenderer) {
        try {
            gModRenderer();
        } catch (...) {
        }
    }

    ImGui::Render();
    gContext->OMSetRenderTargets(1, &gRenderTarget, nullptr);
    ImGui_ImplDX11_RenderDrawData(ImGui::GetDrawData());
}

HRESULT __stdcall DetourPresent(IDXGISwapChain* swapChain, UINT syncInterval, UINT flags) {
    Kuru::ThreadBridge::NotifyPresent();  // drives the game-thread tick (consumed in the PE detour)
    if (!gInitialized) {
        InitializeOverlay(swapChain);
    }
    if (gInitialized) {
        if (!gRenderTarget) {
            CreateRenderTarget(swapChain);
        }
        RenderOverlay();
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
    MH_Initialize();
    if (MH_CreateHook(gPresentTarget, reinterpret_cast<void*>(&DetourPresent),
                      reinterpret_cast<void**>(&gOriginalPresent)) != MH_OK ||
        MH_EnableHook(gPresentTarget) != MH_OK) {
        Log("GUI install failed: Present hook");
        return false;
    }
    if (gResizeTarget) {
        MH_CreateHook(gResizeTarget, reinterpret_cast<void*>(&DetourResizeBuffers),
                      reinterpret_cast<void**>(&gOriginalResizeBuffers));
        MH_EnableHook(gResizeTarget);
    }
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
    if (gPresentTarget) {
        MH_DisableHook(gPresentTarget);
    }
    if (gResizeTarget) {
        MH_DisableHook(gResizeTarget);
    }
    if (gInitialized) {
        if (gWindow && gOriginalWndProc) {
            SetWindowLongPtrW(gWindow, GWLP_WNDPROC, reinterpret_cast<LONG_PTR>(gOriginalWndProc));
        }
        ImGui_ImplDX11_Shutdown();
        ImGui_ImplWin32_Shutdown();
        ImGui::DestroyContext();
    }
    if (gRenderTarget) gRenderTarget->Release();
    if (gContext) gContext->Release();
    if (gDevice) gDevice->Release();
}

} // namespace Kuru::Gui
