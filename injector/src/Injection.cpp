#include "Injection.hpp"

#include <windows.h>
#include <tlhelp32.h>

#include <filesystem>


namespace Kuru::Injector {

namespace {

// RAII wrapper so every early return closes the handle.
struct Handle {
    HANDLE value = nullptr;
    explicit Handle(HANDLE h) : value(h) {}
    ~Handle() {
        if (value && value != INVALID_HANDLE_VALUE) {
            CloseHandle(value);
        }
    }
    Handle(const Handle&) = delete;
    Handle& operator=(const Handle&) = delete;
    [[nodiscard]] bool valid() const { return value && value != INVALID_HANDLE_VALUE; }
};

[[nodiscard]] std::string LastErrorText(const char* what) {
    const DWORD code = GetLastError();
    return std::string(what) + " failed (error " + std::to_string(code) + ")";
}

} // namespace

std::uint32_t FindProcessId(const wchar_t* exeName) {
    Handle snapshot(CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0));
    if (!snapshot.valid()) {
        return 0;
    }

    PROCESSENTRY32W entry{};
    entry.dwSize = sizeof(entry);
    if (!Process32FirstW(snapshot.value, &entry)) {
        return 0;
    }

    do {
        if (_wcsicmp(entry.szExeFile, exeName) == 0) {
            return entry.th32ProcessID;
        }
    } while (Process32NextW(snapshot.value, &entry));

    return 0;
}

InjectResult InjectLibrary(std::uint32_t pid, const std::string& dllPath) {
    std::error_code ec;
    if (dllPath.empty() || !std::filesystem::exists(dllPath, ec)) {
        return {false, "DLL not found: " + dllPath};
    }
    // LoadLibrary needs an absolute path since it runs in the target's working directory.
    const std::string fullPath = std::filesystem::absolute(dllPath, ec).string();
    const std::string& path = ec ? dllPath : fullPath;

    if (pid == 0) {
        return {false, "Target process is not running"};
    }

    Handle process(OpenProcess(
        PROCESS_CREATE_THREAD | PROCESS_QUERY_INFORMATION | PROCESS_VM_OPERATION |
            PROCESS_VM_WRITE | PROCESS_VM_READ,
        FALSE, pid));
    if (!process.valid()) {
        return {false, LastErrorText("OpenProcess") + " - try running as administrator"};
    }

    const SIZE_T size = path.size() + 1;
    void* remote = VirtualAllocEx(process.value, nullptr, size, MEM_COMMIT | MEM_RESERVE, PAGE_READWRITE);
    if (!remote) {
        return {false, LastErrorText("VirtualAllocEx")};
    }

    // Free the remote path buffer on every exit path once it exists.
    struct RemoteFree {
        HANDLE proc;
        void* mem;
        ~RemoteFree() {
            if (mem) {
                VirtualFreeEx(proc, mem, 0, MEM_RELEASE);
            }
        }
    } remoteFree{process.value, remote};

    if (!WriteProcessMemory(process.value, remote, path.c_str(), size, nullptr)) {
        return {false, LastErrorText("WriteProcessMemory")};
    }

    const HMODULE kernel32 = GetModuleHandleW(L"kernel32.dll");
    const auto loadLibrary = reinterpret_cast<LPTHREAD_START_ROUTINE>(
        kernel32 ? GetProcAddress(kernel32, "LoadLibraryA") : nullptr);
    if (!loadLibrary) {
        return {false, LastErrorText("GetProcAddress(LoadLibraryA)")};
    }

    Handle thread(CreateRemoteThread(process.value, nullptr, 0, loadLibrary, remote, 0, nullptr));
    if (!thread.valid()) {
        return {false, LastErrorText("CreateRemoteThread")};
    }

    if (WaitForSingleObject(thread.value, 15000) == WAIT_TIMEOUT) {
        return {false, "Injection timed out waiting for the loader to initialise"};
    }

    // On x64 the thread exit code is the low 32 bits of the HMODULE LoadLibrary returned: zero means the load failed inside the target, non-zero means the module loaded.
    DWORD exitCode = 0;
    GetExitCodeThread(thread.value, &exitCode);
    if (exitCode == 0) {
        return {false, "The game rejected the DLL (LoadLibrary returned null)"};
    }

    return {true, "Injected " + std::filesystem::path(path).filename().string()};
}

bool IsModuleLoaded(std::uint32_t pid, const wchar_t* moduleName) {
    if (pid == 0) {
        return false;
    }
    // TH32CS_SNAPMODULE covers 64-bit modules in a 64-bit target, which is what Rocket League is.
    Handle snapshot(CreateToolhelp32Snapshot(TH32CS_SNAPMODULE, pid));
    if (!snapshot.valid()) {
        return false;
    }

    MODULEENTRY32W entry{};
    entry.dwSize = sizeof(entry);
    if (!Module32FirstW(snapshot.value, &entry)) {
        return false;
    }
    do {
        if (_wcsicmp(entry.szModule, moduleName) == 0) {
            return true;
        }
    } while (Module32NextW(snapshot.value, &entry));

    return false;
}

} // namespace Kuru::Injector
