#include "Config.hpp"
#include "Embedded.hpp"
#include "Injection.hpp"
#include "Mods.hpp"

#include <windows.h>
#include <commdlg.h>
#include <d3d11.h>
#include <shellapi.h>
#include <tchar.h>

#include <array>
#include <cstdio>
#include <filesystem>
#include <fstream>
#include <iterator>
#include <string>
#include <vector>

#include <imgui.h>
#include <backends/imgui_impl_dx11.h>
#include <backends/imgui_impl_win32.h>
#include <IconsLucide.h>

extern IMGUI_IMPL_API LRESULT ImGui_ImplWin32_WndProcHandler(HWND, UINT, WPARAM, LPARAM);

namespace {

// Files dropped onto the window, filled by WM_DROPFILES and drained by the UI each frame (so the window procedure stays decoupled from the UI's current mods folder).
std::vector<std::string> gDroppedFiles;

// Display scale (1.0 at 96 DPI), applied to explicit pixel sizes the style can't scale on its own.
float gDpiScale = 1.0f;

// Persisted settings, loaded at startup and saved on exit. The UI mirrors edits back into it.
Kuru::Injector::Config gConfig;

constexpr const char* kInjectorVersion = "1.0.1";

// --- D3D11 plumbing -------------------------------------------------------------------------------
ID3D11Device* gDevice = nullptr;
ID3D11DeviceContext* gContext = nullptr;
IDXGISwapChain* gSwapChain = nullptr;
ID3D11RenderTargetView* gRenderTarget = nullptr;

void CreateRenderTarget() {
    ID3D11Texture2D* backBuffer = nullptr;
    gSwapChain->GetBuffer(0, IID_PPV_ARGS(&backBuffer));
    if (backBuffer) {
        gDevice->CreateRenderTargetView(backBuffer, nullptr, &gRenderTarget);
        backBuffer->Release();
    }
}

void CleanupRenderTarget() {
    if (gRenderTarget) {
        gRenderTarget->Release();
        gRenderTarget = nullptr;
    }
}

bool CreateDeviceD3D(HWND hwnd) {
    DXGI_SWAP_CHAIN_DESC desc{};
    desc.BufferCount = 2;
    desc.BufferDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
    desc.BufferDesc.RefreshRate.Numerator = 60;
    desc.BufferDesc.RefreshRate.Denominator = 1;
    desc.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
    desc.OutputWindow = hwnd;
    desc.SampleDesc.Count = 1;
    desc.Windowed = TRUE;
    desc.SwapEffect = DXGI_SWAP_EFFECT_DISCARD;

    D3D_FEATURE_LEVEL featureLevel{};
    const std::array levels{D3D_FEATURE_LEVEL_11_0, D3D_FEATURE_LEVEL_10_0};
    if (D3D11CreateDeviceAndSwapChain(
            nullptr, D3D_DRIVER_TYPE_HARDWARE, nullptr, 0, levels.data(),
            static_cast<UINT>(levels.size()), D3D11_SDK_VERSION, &desc, &gSwapChain, &gDevice,
            &featureLevel, &gContext) != S_OK) {
        return false;
    }
    CreateRenderTarget();
    return true;
}

void CleanupDeviceD3D() {
    CleanupRenderTarget();
    if (gSwapChain) { gSwapChain->Release(); gSwapChain = nullptr; }
    if (gContext) { gContext->Release(); gContext = nullptr; }
    if (gDevice) { gDevice->Release(); gDevice = nullptr; }
}

LRESULT WINAPI WndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam) {
    if (ImGui_ImplWin32_WndProcHandler(hwnd, msg, wParam, lParam)) {
        return true;
    }
    switch (msg) {
    case WM_SIZE:
        if (gDevice && wParam != SIZE_MINIMIZED) {
            CleanupRenderTarget();
            gSwapChain->ResizeBuffers(0, LOWORD(lParam), HIWORD(lParam), DXGI_FORMAT_UNKNOWN, 0);
            CreateRenderTarget();
        }
        return 0;
    case WM_SYSCOMMAND:
        if ((wParam & 0xfff0) == SC_KEYMENU) return 0;  // disable ALT menu
        break;
    case WM_DROPFILES: {
        HDROP drop = reinterpret_cast<HDROP>(wParam);
        const UINT count = DragQueryFileW(drop, 0xFFFFFFFF, nullptr, 0);
        for (UINT i = 0; i < count; ++i) {
            wchar_t wide[MAX_PATH]{};
            if (DragQueryFileW(drop, i, wide, MAX_PATH)) {
                const int len = WideCharToMultiByte(CP_UTF8, 0, wide, -1, nullptr, 0, nullptr, nullptr);
                if (len > 1) {
                    std::string narrow(static_cast<std::size_t>(len - 1), '\0');
                    WideCharToMultiByte(CP_UTF8, 0, wide, -1, narrow.data(), len, nullptr, nullptr);
                    gDroppedFiles.push_back(std::move(narrow));
                }
            }
        }
        DragFinish(drop);
        return 0;
    }
    case WM_DESTROY:
        PostQuitMessage(0);
        return 0;
    }
    return DefWindowProcW(hwnd, msg, wParam, lParam);
}

// --- Kuru UI --------------------------------------------------------------------------------------
constexpr wchar_t kTargetProcess[] = L"RocketLeague.exe";

std::string ExeDirectory() {
    wchar_t buffer[MAX_PATH]{};
    const DWORD len = GetModuleFileNameW(nullptr, buffer, MAX_PATH);
    if (len == 0) {
        return {};
    }
    return std::filesystem::path(std::wstring(buffer, len)).parent_path().string();
}

// Path to a font in the Windows Fonts directory, e.g. "segoeui.ttf".
std::string SystemFontPath(const char* fileName) {
    char windir[MAX_PATH]{};
    if (GetWindowsDirectoryA(windir, MAX_PATH) == 0) {
        return {};
    }
    return std::string(windir) + "\\Fonts\\" + fileName;
}

// %APPDATA%\Kuru\bin, where the embedded DLLs are extracted to run from.
std::filesystem::path KuruBinDir() {
    const char* appData = std::getenv("APPDATA");
    if (!appData || *appData == '\0') {
        return {};
    }
    return std::filesystem::path(appData) / "Kuru" / "bin";
}

// Write data to path unless a file of the same size is already there
void WriteEmbedded(const std::filesystem::path& path, const unsigned char* data, unsigned int size) {
    if (size == 0) {
        return;
    }
    std::error_code ec;
    if (std::filesystem::exists(path, ec) && std::filesystem::file_size(path, ec) == size) {
        return;
    }
    std::ofstream out(path, std::ios::binary | std::ios::trunc);
    if (out) {
        out.write(reinterpret_cast<const char*>(data), size);
    }
}

// Extract the embedded DLLs to %APPDATA%\Kuru\bin and return that directory.
std::filesystem::path ExtractBundledDlls() {
    const std::filesystem::path dir = KuruBinDir();
    if (dir.empty()) {
        return {};
    }
    std::error_code ec;
    std::filesystem::create_directories(dir, ec);
    WriteEmbedded(dir / "KuruLoader.dll", KURU_LOADER_DATA, KURU_LOADER_DATA_size);
    WriteEmbedded(dir / "KuruSDKGen.dll", KURU_SDKGEN_DATA, KURU_SDKGEN_DATA_size);
    return dir;
}

// Read the last maxBytes of a file (for showing a log tail). Returns empty if it can't be read.
std::string ReadFileTail(const std::filesystem::path& path, std::size_t maxBytes) {
    std::error_code ec;
    if (path.empty() || !std::filesystem::exists(path, ec)) {
        return {};
    }
    std::ifstream in(path, std::ios::binary | std::ios::ate);
    if (!in) {
        return {};
    }
    const std::streamoff size = in.tellg();
    const std::streamoff start = size > static_cast<std::streamoff>(maxBytes)
                                     ? size - static_cast<std::streamoff>(maxBytes)
                                     : 0;
    in.seekg(start);
    std::string content((std::istreambuf_iterator<char>(in)), std::istreambuf_iterator<char>());
    if (start > 0) {
        const auto nl = content.find('\n');  // drop the partial first line
        if (nl != std::string::npos) {
            content.erase(0, nl + 1);
        }
    }
    return content;
}

// Copy up to the buffer capacity, always null-terminated.
void SetField(std::array<char, MAX_PATH>& field, const std::string& value) {
    const std::size_t n = value.copy(field.data(), field.size() - 1);
    field[n] = '\0';
}

// Default a DLL path to <exe dir>\name if that file is present next to the injector (the release layout).
void DefaultDllPath(std::array<char, MAX_PATH>& field, const std::string& exeDir, const char* name) {
    if (exeDir.empty()) {
        return;
    }
    const std::filesystem::path candidate = std::filesystem::path(exeDir) / name;
    std::error_code ec;
    if (std::filesystem::exists(candidate, ec)) {
        SetField(field, candidate.string());
    }
}

void BrowseForDll(HWND owner, std::array<char, MAX_PATH>& field) {
    char file[MAX_PATH]{};
    field[field.size() - 1] = '\0';
    std::snprintf(file, sizeof(file), "%s", field.data());

    OPENFILENAMEA ofn{};
    ofn.lStructSize = sizeof(ofn);
    ofn.hwndOwner = owner;
    ofn.lpstrFilter = "DLL files\0*.dll\0All files\0*.*\0";
    ofn.lpstrFile = file;
    ofn.nMaxFile = sizeof(file);
    ofn.Flags = OFN_FILEMUSTEXIST | OFN_PATHMUSTEXIST | OFN_NOCHANGEDIR;
    if (GetOpenFileNameA(&ofn)) {
        SetField(field, file);
    }
}

// Build an opaque ImGui colour from a 0xRRGGBB literal so the palette reads like the design spec.
ImVec4 Col(unsigned int hex) {
    return ImVec4(((hex >> 16) & 0xFF) / 255.0f, ((hex >> 8) & 0xFF) / 255.0f, (hex & 0xFF) / 255.0f, 1.0f);
}

// A rounded pill badge with a coloured status dot
void StatusPill(bool running, std::uint32_t pid) {
    ImDrawList* dl = ImGui::GetWindowDrawList();
    const ImVec2 origin = ImGui::GetCursorScreenPos();
    const char* label = running ? "RL Running" : "Waiting for game";
    const ImU32 dotColor = running ? IM_COL32(52, 211, 132, 255) : IM_COL32(214, 178, 94, 255);

    const float padX = 12.0f * gDpiScale;
    const float dotR = 5.0f * gDpiScale;
    const float gap = 9.0f * gDpiScale;
    const ImVec2 ts = ImGui::CalcTextSize(label);
    const float h = ts.y + 12.0f * gDpiScale;
    const float w = padX + dotR * 2.0f + gap + ts.x + padX;
    const ImVec2 mn = origin;
    const ImVec2 mx = ImVec2(origin.x + w, origin.y + h);

    dl->AddRectFilled(mn, mx, IM_COL32(27, 29, 38, 255), h * 0.5f);
    dl->AddRect(mn, mx, IM_COL32(40, 42, 54, 255), h * 0.5f);

    const ImVec2 c = ImVec2(mn.x + padX + dotR, (mn.y + mx.y) * 0.5f);
    if (running) {
        dl->AddCircleFilled(c, dotR * 2.4f, IM_COL32(52, 211, 132, 55));  // outer glow
        dl->AddCircleFilled(c, dotR * 1.6f, IM_COL32(52, 211, 132, 90));  // inner glow
    }
    dl->AddCircleFilled(c, dotR, dotColor);
    dl->AddText(ImVec2(c.x + dotR + gap, mn.y + (h - ts.y) * 0.5f), IM_COL32(226, 227, 234, 255), label);

    ImGui::Dummy(ImVec2(w, h));
    if (running) {
        ImGui::SameLine();
        ImGui::AlignTextToFramePadding();
        ImGui::TextDisabled("PID %u", pid);
    }
}

void RenderUi(HWND hwnd) {
    using namespace Kuru::Injector;

    // Persistent UI state.
    static bool initialised = false;
    static std::array<char, MAX_PATH> loaderPath{};
    static std::array<char, MAX_PATH> generatorPath{};
    static std::string status = "Ready.";
    static bool statusIsError = false;

    if (!initialised) {
        const std::string exeDir = ExeDirectory();
        // Prefer saved paths, fall back to a DLL sitting next to the injector.
        if (!gConfig.loaderPath.empty()) {
            SetField(loaderPath, gConfig.loaderPath);
        } else {
            DefaultDllPath(loaderPath, exeDir, "KuruLoader.dll");
        }
        if (!gConfig.generatorPath.empty()) {
            SetField(generatorPath, gConfig.generatorPath);
        } else {
            DefaultDllPath(generatorPath, exeDir, "KuruSDKGen.dll");
        }
        initialised = true;
    }

    // Mirror the editable paths back into the config each frame so they persist on exit.
    gConfig.loaderPath = loaderPath.data();
    gConfig.generatorPath = generatorPath.data();

    // Process + loader-module polling, refreshed roughly twice a second rather than every frame.
    static std::uint32_t pid = 0;
    static bool loaderInjected = false;
    static double lastPoll = -1.0;
    const double now = ImGui::GetTime();
    if (now - lastPoll > 0.5) {
        pid = FindProcessId(kTargetProcess);
        loaderInjected = pid != 0 && IsModuleLoaded(pid, L"KuruLoader.dll");
        lastPoll = now;
    }
    const bool running = pid != 0;

    // Auto-inject the loader once per game session when enabled.
    static bool autoInjectedThisSession = false;
    if (!running) {
        autoInjectedThisSession = false;
    } else if (gConfig.autoInject && !loaderInjected && !autoInjectedThisSession && loaderPath[0] != '\0') {
        const InjectResult result = InjectLibrary(pid, loaderPath.data());
        status = result.message;
        statusIsError = !result.ok;
        autoInjectedThisSession = true;
    }

    // The mods folder the loader scans, derived from the configured loader DLL path.
    const std::filesystem::path modsDir = ModsDirForLoader(loaderPath.data());

    // Install anything dropped onto the window since last frame.
    if (!gDroppedFiles.empty()) {
        int installed = 0;
        std::string lastError;
        for (const std::string& file : gDroppedFiles) {
            std::string err;
            if (InstallMod(modsDir, file, err)) {
                ++installed;
            } else {
                lastError = err;
            }
        }
        gDroppedFiles.clear();
        if (installed > 0) {
            status = "Installed " + std::to_string(installed) + " mod" + (installed == 1 ? "" : "s");
            statusIsError = false;
        } else if (!lastError.empty()) {
            status = lastError;
            statusIsError = true;
        }
    }

    // One window filling the OS client area; the OS title bar provides the chrome.
    const ImGuiViewport* viewport = ImGui::GetMainViewport();
    ImGui::SetNextWindowPos(viewport->WorkPos);
    ImGui::SetNextWindowSize(viewport->WorkSize);
    ImGui::Begin("Kuru", nullptr,
                 ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoMove |
                     ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoBringToFrontOnFocus |
                     ImGuiWindowFlags_NoScrollbar);

    const ImVec4 red(0.90f, 0.42f, 0.42f, 1.0f);

    // Header: wordmark + version, then the status pill on its own line.
    ImGui::TextColored(Col(0x8B5CF6), ICON_LC_ROCKET "  KURU");
    ImGui::SameLine(0.0f, 8.0f);
    ImGui::TextColored(Col(0x6366F1), "INJECTOR");
    ImGui::SameLine(0.0f, 8.0f);
    ImGui::TextDisabled("v%s", kInjectorVersion);
    ImGui::Spacing();
    StatusPill(running, pid);

    ImGui::Dummy(ImVec2(0.0f, 6.0f * gDpiScale));

    auto dllRow = [&](const char* label, std::array<char, MAX_PATH>& field, const char* id) {
        ImGui::TextUnformatted(label);
        ImGui::SetNextItemWidth(ImGui::GetContentRegionAvail().x - 90.0f * gDpiScale);
        ImGui::InputText((std::string("##path") + id).c_str(), field.data(), field.size());
        ImGui::SameLine();
        if (ImGui::Button((std::string("Browse##") + id).c_str(), ImVec2(80.0f * gDpiScale, 0.0f))) {
            BrowseForDll(hwnd, field);
        }
    };

    auto injectButton = [&](const char* label, std::array<char, MAX_PATH>& field) {
        ImGui::BeginDisabled(!running);
        if (ImGui::Button(label, ImVec2(-FLT_MIN, 36.0f * gDpiScale))) {
            const InjectResult result = InjectLibrary(pid, field.data());
            status = result.message;
            statusIsError = !result.ok;
        }
        ImGui::EndDisabled();
    };

    // Each section sits in its own elevated card.
    auto beginCard = [&](const char* id) {
        ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(14.0f * gDpiScale, 12.0f * gDpiScale));
        ImGui::BeginChild(id, ImVec2(0.0f, 0.0f), ImGuiChildFlags_Borders | ImGuiChildFlags_AutoResizeY);
    };
    auto endCard = [&]() {
        ImGui::EndChild();
        ImGui::PopStyleVar();
    };

    if (ImGui::BeginTabBar("##tabs")) {
        // The everyday tab: load the mod loader and manage installed mods.
        if (ImGui::BeginTabItem(ICON_LC_PACKAGE " Mods")) {
            ImGui::Spacing();

            beginCard("loaderCard");
            dllRow("Mod loader (KuruLoader.dll)", loaderPath, "loader");
            ImGui::Spacing();
            if (loaderInjected) {
                ImGui::BeginDisabled(true);
                ImGui::Button(ICON_LC_CIRCLE_CHECK " Loader already active in game",
                              ImVec2(-FLT_MIN, 36.0f * gDpiScale));
                ImGui::EndDisabled();
            } else {
                injectButton(ICON_LC_DOWNLOAD " Inject mod loader", loaderPath);
            }
            ImGui::Spacing();
            ImGui::Checkbox("Auto-inject when Rocket League starts", &gConfig.autoInject);
            endCard();

            ImGui::Dummy(ImVec2(0.0f, 8.0f * gDpiScale));

            beginCard("modsCard");
            const std::vector<ModEntry> mods = ListMods(modsDir);
            ImGui::AlignTextToFramePadding();
            ImGui::TextUnformatted(ICON_LC_PACKAGE " Installed mods");
            ImGui::SameLine();
            ImGui::TextDisabled("(%d)", static_cast<int>(mods.size()));
            {
                const float bw = 124.0f * gDpiScale;
                ImGui::SameLine();
                ImGui::SetCursorPosX(ImGui::GetCursorPosX() + ImGui::GetContentRegionAvail().x - bw);
                ImGui::BeginDisabled(modsDir.empty());
                if (ImGui::Button(ICON_LC_FOLDER_OPEN " Open folder", ImVec2(bw, 0.0f))) {
                    OpenInExplorer(modsDir);
                }
                ImGui::EndDisabled();
            }
            ImGui::TextDisabled("Drag .dll files onto the window to install.");
            ImGui::Separator();

            if (modsDir.empty()) {
                ImGui::TextDisabled("Set the mod loader path above to manage mods.");
            } else if (mods.empty()) {
                ImGui::TextDisabled("No mods installed yet.");
            } else {
                for (const ModEntry& mod : mods) {
                    ImGui::PushID(mod.name.c_str());

                    bool enabled = mod.enabled;
                    if (ImGui::Checkbox("##enabled", &enabled)) {
                        std::string err;
                        if (!SetModEnabled(mod, enabled, err)) {
                            status = err;
                            statusIsError = true;
                        }
                    }
                    ImGui::SameLine();
                    ImGui::AlignTextToFramePadding();
                    if (mod.enabled) {
                        ImGui::TextUnformatted(mod.name.c_str());
                    } else {
                        ImGui::TextDisabled("%s (disabled)", mod.name.c_str());
                    }

                    const float rw = 92.0f * gDpiScale;
                    ImGui::SameLine();
                    ImGui::SetCursorPosX(ImGui::GetCursorPosX() + ImGui::GetContentRegionAvail().x - rw);
                    if (ImGui::Button(ICON_LC_TRASH_2 " Remove", ImVec2(rw, 0.0f))) {
                        ImGui::OpenPopup("confirm_remove");
                    }

                    if (ImGui::BeginPopupModal("confirm_remove", nullptr, ImGuiWindowFlags_AlwaysAutoResize)) {
                        ImGui::Text("Remove \"%s\"?", mod.name.c_str());
                        ImGui::Spacing();
                        if (ImGui::Button("Remove", ImVec2(120.0f * gDpiScale, 0.0f))) {
                            std::string err;
                            if (RemoveMod(mod, err)) {
                                status = "Removed " + mod.name;
                                statusIsError = false;
                            } else {
                                status = err;
                                statusIsError = true;
                            }
                            ImGui::CloseCurrentPopup();
                        }
                        ImGui::SameLine();
                        if (ImGui::Button("Cancel", ImVec2(120.0f * gDpiScale, 0.0f))) {
                            ImGui::CloseCurrentPopup();
                        }
                        ImGui::EndPopup();
                    }

                    ImGui::PopID();
                }
            }
            endCard();

            ImGui::EndTabItem();
        }

        // Tucked away so normal users never need it: regenerate the SDK (only after a game update).
        if (ImGui::BeginTabItem(ICON_LC_CODE " Developer")) {
            ImGui::Spacing();

            const std::filesystem::path genDir =
                generatorPath[0] ? std::filesystem::path(generatorPath.data()).parent_path()
                                 : std::filesystem::path{};
            const std::filesystem::path sdkDir = genDir.empty() ? genDir : genDir / "generated-sdk";
            const std::filesystem::path logPath = genDir.empty() ? genDir : genDir / "KuruSDKGen.log";

            beginCard("genCard");
            ImGui::TextWrapped(
                "Generates the typed SDK and a self-contained mod template from the running game. "
                "Use it to start a mod or refresh its SDK after a Rocket League update.");
            ImGui::Spacing();
            dllRow("SDK generator (KuruSDKGen.dll)", generatorPath, "gen");
            ImGui::Spacing();
            injectButton(ICON_LC_CODE " Generate SDK", generatorPath);
            ImGui::Spacing();
            {
                std::error_code ec2;
                const bool haveSdk = !sdkDir.empty() && std::filesystem::exists(sdkDir, ec2);
                ImGui::BeginDisabled(!haveSdk);
                if (ImGui::Button(ICON_LC_FOLDER_OPEN " Open generated-sdk", ImVec2(-FLT_MIN, 0.0f))) {
                    OpenInExplorer(sdkDir);
                }
                ImGui::EndDisabled();
            }
            endCard();

            ImGui::Dummy(ImVec2(0.0f, 8.0f * gDpiScale));

            beginCard("logCard");
            ImGui::TextUnformatted(ICON_LC_CODE " Generation log");
            static std::string logText;
            static double lastLogPoll = -1.0;
            if (now - lastLogPoll > 1.0) {
                logText = ReadFileTail(logPath, 32 * 1024);
                lastLogPoll = now;
            }
            ImGui::BeginChild("logscroll", ImVec2(0.0f, 150.0f * gDpiScale), ImGuiChildFlags_Borders,
                              ImGuiWindowFlags_HorizontalScrollbar);
            if (logText.empty()) {
                ImGui::TextDisabled("No log yet. Generate the SDK to see progress here.");
            } else {
                const bool atBottom = ImGui::GetScrollY() >= ImGui::GetScrollMaxY() - 1.0f;
                ImGui::TextUnformatted(logText.c_str());
                if (atBottom) {
                    ImGui::SetScrollHereY(1.0f);  // follow the tail unless the user scrolled up
                }
            }
            ImGui::EndChild();
            endCard();

            ImGui::EndTabItem();
        }
        if (ImGui::BeginTabItem("Licenses")) {
            ImGui::BeginChild("licenses", ImVec2(0.0f, -ImGui::GetFrameHeightWithSpacing()),
                              ImGuiChildFlags_None, ImGuiWindowFlags_HorizontalScrollbar);
            const char* text = reinterpret_cast<const char*>(KURU_NOTICES_DATA);
            ImGui::TextUnformatted(text, text + KURU_NOTICES_DATA_size);
            ImGui::EndChild();
            ImGui::EndTabItem();
        }
        ImGui::EndTabBar();
    }

    // Status line pinned to the bottom.
    ImGui::SetCursorPosY(ImGui::GetWindowHeight() - ImGui::GetFrameHeightWithSpacing());
    ImGui::Separator();
    ImGui::TextColored(statusIsError ? red : Col(0xB8BAC6), "%s", status.c_str());

    ImGui::End();
}

void ApplyTheme() {
    ImGui::StyleColorsDark();
    ImGuiStyle& style = ImGui::GetStyle();
    style.WindowRounding = 0.0f;
    style.ChildRounding = 8.0f;
    style.FrameRounding = 6.0f;
    style.GrabRounding = 6.0f;
    style.TabRounding = 6.0f;
    style.PopupRounding = 6.0f;
    style.ScrollbarRounding = 6.0f;
    style.WindowBorderSize = 0.0f;
    style.ChildBorderSize = 1.0f;
    style.FrameBorderSize = 0.0f;
    style.FramePadding = ImVec2(10.0f, 7.0f);
    style.ItemSpacing = ImVec2(10.0f, 9.0f);
    style.WindowPadding = ImVec2(16.0f, 14.0f);

    ImVec4* colors = style.Colors;
    const ImVec4 accent = Col(0x6366F1);        // indigo
    const ImVec4 accentHover = Col(0x7C7FF5);
    const ImVec4 accentActive = Col(0x4F52D9);
    const ImVec4 border = Col(0x282A36);

    colors[ImGuiCol_WindowBg] = Col(0x0D0E12);          // deep main background
    colors[ImGuiCol_ChildBg] = Col(0x16171F);           // elevated cards
    colors[ImGuiCol_PopupBg] = Col(0x16171F);
    colors[ImGuiCol_Border] = border;
    colors[ImGuiCol_FrameBg] = Col(0x20222E);
    colors[ImGuiCol_FrameBgHovered] = Col(0x2A2C3A);
    colors[ImGuiCol_FrameBgActive] = Col(0x2A2C3A);
    colors[ImGuiCol_Button] = accent;
    colors[ImGuiCol_ButtonHovered] = accentHover;
    colors[ImGuiCol_ButtonActive] = accentActive;
    colors[ImGuiCol_Separator] = border;
    colors[ImGuiCol_SeparatorHovered] = accent;
    colors[ImGuiCol_CheckMark] = accent;
    colors[ImGuiCol_SliderGrab] = accent;
    colors[ImGuiCol_Header] = Col(0x20222E);
    colors[ImGuiCol_HeaderHovered] = accentHover;
    colors[ImGuiCol_HeaderActive] = accentActive;
    colors[ImGuiCol_Tab] = Col(0x16171F);
    colors[ImGuiCol_TabHovered] = accentHover;
    colors[ImGuiCol_TabActive] = Col(0x20222E);
    colors[ImGuiCol_TabUnfocused] = Col(0x16171F);
    colors[ImGuiCol_TabUnfocusedActive] = Col(0x20222E);
    colors[ImGuiCol_Text] = Col(0xE4E4EA);
    colors[ImGuiCol_TextDisabled] = Col(0x6B6E7B);
    colors[ImGuiCol_ScrollbarBg] = Col(0x0D0E12);
    colors[ImGuiCol_ScrollbarGrab] = Col(0x282A36);
}

} // namespace

int APIENTRY wWinMain(HINSTANCE instance, HINSTANCE, PWSTR, int) {
    // Render at native resolution instead of letting Windows bitmap-stretch the window (which looks
    // blurry on high-DPI displays).
    SetProcessDpiAwarenessContext(DPI_AWARENESS_CONTEXT_PER_MONITOR_AWARE_V2);

    gConfig = Kuru::Injector::LoadConfig();

    // Unpack the bundled DLLs and point the (unset) paths at them, so a freshly downloaded single exe
    // just works with no files to place.
    const std::filesystem::path binDir = ExtractBundledDlls();
    if (!binDir.empty()) {
        if (gConfig.loaderPath.empty()) {
            gConfig.loaderPath = (binDir / "KuruLoader.dll").string();
        }
        if (gConfig.generatorPath.empty()) {
            gConfig.generatorPath = (binDir / "KuruSDKGen.dll").string();
        }
    }

    WNDCLASSEXW wc{};
    wc.cbSize = sizeof(wc);
    wc.style = CS_CLASSDC;
    wc.lpfnWndProc = WndProc;
    wc.hInstance = instance;
    wc.hCursor = LoadCursor(nullptr, IDC_ARROW);
    wc.lpszClassName = L"KuruInjector";
    RegisterClassExW(&wc);

    const float initScale = GetDpiForSystem() / 96.0f;
    const bool haveGeom = gConfig.winW > 0 && gConfig.winH > 0;
    const int wx = (haveGeom && gConfig.winX != INT_MIN) ? gConfig.winX : CW_USEDEFAULT;
    const int wy = (haveGeom && gConfig.winY != INT_MIN) ? gConfig.winY : CW_USEDEFAULT;
    const int ww = haveGeom ? gConfig.winW : static_cast<int>(540 * initScale);
    const int wh = haveGeom ? gConfig.winH : static_cast<int>(580 * initScale);
    HWND hwnd = CreateWindowW(wc.lpszClassName, L"Kuru Injector", WS_OVERLAPPEDWINDOW, wx, wy, ww, wh,
                              nullptr, nullptr, instance, nullptr);

    if (!CreateDeviceD3D(hwnd)) {
        CleanupDeviceD3D();
        UnregisterClassW(wc.lpszClassName, instance);
        MessageBoxA(nullptr, "Failed to create the Direct3D device.", "Kuru Injector", MB_ICONERROR);
        return 1;
    }

    ShowWindow(hwnd, SW_SHOWDEFAULT);
    UpdateWindow(hwnd);
    DragAcceptFiles(hwnd, TRUE);  // enable drag-and-drop mod install

    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGuiIO& io = ImGui::GetIO();
    io.IniFilename = nullptr;  // no imgui.ini next to the exe

    // Load fonts at the monitor's real pixel size so text is crisp rather than upscaled.
    const float dpiScale = GetDpiForWindow(hwnd) / 96.0f;
    gDpiScale = dpiScale;
    const float fontSize = 16.0f * dpiScale;
    std::error_code ec;

    const std::string uiFont = SystemFontPath("segoeui.ttf");  // clean, anti-aliased, on every Windows
    if (!uiFont.empty() && std::filesystem::exists(uiFont, ec)) {
        io.Fonts->AddFontFromFileTTF(uiFont.c_str(), fontSize);
    } else {
        io.Fonts->AddFontDefault();
    }

    // Merge the Lucide icon glyphs in at the same size, from the font embedded in the exe.
    if (KURU_ICONFONT_DATA_size > 0) {
        static const ImWchar range[] = {ICON_MIN_LC, ICON_MAX_16_LC, 0};
        ImFontConfig cfg;
        cfg.MergeMode = true;
        cfg.PixelSnapH = true;
        cfg.GlyphMinAdvanceX = fontSize;
        cfg.GlyphOffset.y = 3.0f * dpiScale;    // sit icons on the text baseline
        cfg.FontDataOwnedByAtlas = false;       // static array; ImGui must not free it
        io.Fonts->AddFontFromMemoryTTF(const_cast<unsigned char*>(KURU_ICONFONT_DATA),
                                       static_cast<int>(KURU_ICONFONT_DATA_size), fontSize, &cfg, range);
    }

    ApplyTheme();
    ImGui::GetStyle().ScaleAllSizes(dpiScale);  // padding, rounding, etc. scale with DPI too
    ImGui_ImplWin32_Init(hwnd);
    ImGui_ImplDX11_Init(gDevice, gContext);

    bool done = false;
    while (!done) {
        MSG msg;
        while (PeekMessageW(&msg, nullptr, 0, 0, PM_REMOVE)) {
            TranslateMessage(&msg);
            DispatchMessageW(&msg);
            if (msg.message == WM_QUIT) {
                done = true;
            }
        }
        if (done) {
            break;
        }

        ImGui_ImplDX11_NewFrame();
        ImGui_ImplWin32_NewFrame();
        ImGui::NewFrame();

        RenderUi(hwnd);

        ImGui::Render();
        const float clear[4] = {0.06f, 0.06f, 0.07f, 1.0f};
        gContext->OMSetRenderTargets(1, &gRenderTarget, nullptr);
        gContext->ClearRenderTargetView(gRenderTarget, clear);
        ImGui_ImplDX11_RenderDrawData(ImGui::GetDrawData());
        gSwapChain->Present(1, 0);  // vsync
    }

    // Persist window geometry and settings before tearing down.
    RECT rect{};
    if (GetWindowRect(hwnd, &rect)) {
        gConfig.winX = rect.left;
        gConfig.winY = rect.top;
        gConfig.winW = rect.right - rect.left;
        gConfig.winH = rect.bottom - rect.top;
    }
    Kuru::Injector::SaveConfig(gConfig);

    ImGui_ImplDX11_Shutdown();
    ImGui_ImplWin32_Shutdown();
    ImGui::DestroyContext();
    CleanupDeviceD3D();
    DestroyWindow(hwnd);
    UnregisterClassW(wc.lpszClassName, instance);
    return 0;
}
