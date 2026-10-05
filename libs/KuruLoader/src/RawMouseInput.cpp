#include "RawMouseInput.hpp"
#include "RawMousePolicy.hpp"
#include "Hook.hpp"
#include "Lifecycle.hpp"
#include "Console.hpp"
#include <windows.h>
#include <atomic>
#include <vector>

namespace Kuru::RawMouseInput {
namespace {
THook windowHook, dataHook;
Lifecycle::Activity activity;
WNDPROC originalWindow = nullptr;
decltype(&GetRawInputData) originalData = nullptr;
std::atomic<bool> capturing{false};
std::atomic<bool> reportedFilter{false};
// The raw thread never accesses ImGui. Per-message decisions survive repeated reads.
struct Decision { HRAWINPUT handle = nullptr; USHORT remove = 0; bool motion = false; };
thread_local Decision decision;
thread_local USHORT ownedButtons = 0;

UINT WINAPI Read(HRAWINPUT handle, UINT command, LPVOID data, PUINT size, UINT headerSize) {
    Lifecycle::Activity::Scope scope(activity);
    const UINT bytes = originalData(handle, command, data, size, headerSize);
    if (Lifecycle::stopping.load() || command != RID_INPUT || !data || bytes == UINT(-1) ||
        bytes < sizeof(RAWINPUT) || handle != decision.handle) return bytes;
    auto* input = static_cast<RAWINPUT*>(data);
    if (input->header.dwType != RIM_TYPEMOUSE) return bytes;
    if ((decision.remove || decision.motion) && !reportedFilter.exchange(true))
        Console::AddLog("raw mouse capture: filtering RL's GetRawInputData packets");
    input->data.mouse.usButtonFlags &= ~decision.remove;
    if (decision.motion) {
        input->data.mouse.lLastX = input->data.mouse.lLastY = 0;
        input->data.mouse.usButtonFlags &= ~(RI_MOUSE_WHEEL | RI_MOUSE_HWHEEL);
        input->data.mouse.usButtonData = 0;
    }
    return bytes;
}

LRESULT CALLBACK Window(HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam) {
    Lifecycle::Activity::Scope scope(activity);
    if (Lifecycle::stopping.load() || message != WM_INPUT)
        return originalWindow(hwnd, message, wParam, lParam);
    wchar_t className[64]{};
    GetClassNameW(hwnd, className, 64);
    if (lstrcmpW(className, L"RLRawMouseInput") != 0)
        return originalWindow(hwnd, message, wParam, lParam);
    RAWINPUT input{};
    UINT size = sizeof(input);
    const auto handle = reinterpret_cast<HRAWINPUT>(lParam);
    if (originalData(handle, RID_INPUT, &input, &size, sizeof(RAWINPUTHEADER)) == UINT(-1) ||
        input.header.dwType != RIM_TYPEMOUSE)
        return originalWindow(hwnd, message, wParam, lParam);
    Decision next{handle, 0, capturing.load(std::memory_order_acquire)};
    next.remove = SuppressButtons(input.data.mouse.usButtonFlags, next.motion, ownedButtons);
    const auto previous = decision;
    decision = next;
    const auto result = originalWindow(hwnd, message, wParam, lParam);
    decision = previous;
    return result;
}
}
bool Install() {
    UINT count = 0;
    if (GetRegisteredRawInputDevices(nullptr, &count, sizeof(RAWINPUTDEVICE)) == UINT(-1)) return false;
    std::vector<RAWINPUTDEVICE> devices(count);
    if (!count || GetRegisteredRawInputDevices(devices.data(), &count, sizeof(RAWINPUTDEVICE)) == UINT(-1)) return false;
    for (const auto& device : devices) {
        if (device.usUsagePage != 1 || device.usUsage != 2 || !device.hwndTarget) continue;
        DWORD process = 0;
        GetWindowThreadProcessId(device.hwndTarget, &process);
        wchar_t className[64]{};
        GetClassNameW(device.hwndTarget, className, 64);
        if (process != GetCurrentProcessId() || lstrcmpW(className, L"RLRawMouseInput") != 0) continue;
        auto* target = reinterpret_cast<void*>(GetWindowLongPtrW(device.hwndTarget, GWLP_WNDPROC));
        HMODULE owner = nullptr;
        if (!target || !GetModuleHandleExW(GET_MODULE_HANDLE_EX_FLAG_FROM_ADDRESS |
                GET_MODULE_HANDLE_EX_FLAG_UNCHANGED_REFCOUNT, reinterpret_cast<LPCWSTR>(target), &owner) ||
                owner != GetModuleHandleW(nullptr)) continue;
        auto* reader = reinterpret_cast<void*>(GetProcAddress(GetModuleHandleW(L"user32.dll"), "GetRawInputData"));
        if (!dataHook.Install(reader, reinterpret_cast<void*>(&Read), reinterpret_cast<void**>(&originalData))) return false;
        if (!windowHook.Install(target, reinterpret_cast<void*>(&Window), reinterpret_cast<void**>(&originalWindow))) {
            // Keep the reader trampoline alive until orderly teardown; it is pure passthrough outside Window.
            return false;
        }
        Console::AddLog("raw mouse capture installed on RLRawMouseInput");
        return true;
    }
    return false;
}
void Capture(bool mouse) { capturing.store(mouse, std::memory_order_release); }
bool Disable() {
    capturing.store(false);
    const bool window = windowHook.Disable();
    const bool data = dataHook.Disable();
    return window && data;
}
bool WaitForIdle() { return activity.WaitForIdle(); }
void Shutdown() { windowHook.Remove(); dataHook.Remove(); }
}
