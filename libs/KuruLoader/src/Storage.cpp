#include "Storage.hpp"

#include <windows.h>

namespace Kuru::Storage {
namespace {

// Roaming AppData for the current user
std::filesystem::path RoamingAppData() {
    wchar_t buffer[MAX_PATH]{};
    const DWORD len = GetEnvironmentVariableW(L"APPDATA", buffer, MAX_PATH);
    if (len == 0 || len >= MAX_PATH) {
        return std::filesystem::path(L".");
    }
    return std::filesystem::path(buffer, buffer + len);
}

} // namespace

const std::filesystem::path& KuruDir() {
    static const std::filesystem::path dir = [] {
        std::filesystem::path d = RoamingAppData() / "Kuru";
        std::error_code ec;
        std::filesystem::create_directories(d, ec);
        return d;
    }();
    return dir;
}

const std::filesystem::path& GameDir() {
    static const std::filesystem::path dir = [] {
        wchar_t buffer[MAX_PATH]{};
        const DWORD len = GetModuleFileNameW(nullptr, buffer, MAX_PATH);
        if (len == 0 || len == MAX_PATH) {
            return std::filesystem::path();
        }
        return std::filesystem::path(buffer).parent_path();
    }();
    return dir;
}

std::filesystem::path ModDataDir(const char* modName) {
    std::filesystem::path d = KuruDir() / "data" / (modName ? modName : "");
    std::error_code ec;
    std::filesystem::create_directories(d, ec);
    return d;
}

const char* HostGetKuruDir() {
    static const std::string result = KuruDir().string();
    return result.c_str();
}

const char* HostGetGameDir() {
    static const std::string result = GameDir().string();
    return result.c_str();
}

const char* HostGetModDataDir(const char* modName) {
    static thread_local std::string result;
    result = ModDataDir(modName).string();
    return result.c_str();
}

} // namespace Kuru::Storage
