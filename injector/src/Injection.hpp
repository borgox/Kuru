#pragma once

#include <cstdint>
#include <string>

namespace Kuru::Injector {

// Result of an injection attempt: a flag plus a human-readable message for the UI.
struct InjectResult {
    bool ok = false;
    std::string message;
};

// Return the PID of the first process whose executable name matches exeName (case-insensitive)
[[nodiscard]] std::uint32_t FindProcessId(const wchar_t* exeName);

// Inject the DLL at dllPath into the process pid via the classic LoadLibrary remote-thread technique.
[[nodiscard]] InjectResult InjectLibrary(std::uint32_t pid, const std::string& dllPath);

// True if a module named moduleName (case-insensitive, e.g. L"KuruLoader.dll") is already loaded in the process, so we can avoid injecting the same DLL twice.
[[nodiscard]] bool IsModuleLoaded(std::uint32_t pid, const wchar_t* moduleName);

} // namespace Kuru::Injector
