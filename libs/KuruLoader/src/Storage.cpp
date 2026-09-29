#include "Storage.hpp"

#include <windows.h>

#include <cstdlib>
#include <filesystem>

namespace Kuru::Storage {

const std::string& KuruDir() {
    static const std::string dir = [] {
        const char* appData = std::getenv("APPDATA");
        std::string d = appData ? std::string(appData) + "\\Kuru" : std::string(".\\Kuru");
        CreateDirectoryA(d.c_str(), nullptr);
        return d;
    }();
    return dir;
}

const std::string& GameDir() {
    static const std::string dir = [] {
        wchar_t buffer[MAX_PATH]{};
        const DWORD len = GetModuleFileNameW(nullptr, buffer, MAX_PATH);
        if (len == 0 || len == MAX_PATH) {
            return std::string();
        }
        return std::filesystem::path(buffer).parent_path().string();
    }();
    return dir;
}

std::string ModDataDir(const char* modName) {
    std::string d = KuruDir() + "\\data\\" + (modName ? modName : "");
    std::error_code ec;
    std::filesystem::create_directories(d, ec);
    return d;
}

const char* HostGetKuruDir() {
    return KuruDir().c_str();
}

const char* HostGetGameDir() {
    return GameDir().c_str();
}

const char* HostGetModDataDir(const char* modName) {
    static thread_local std::string result;
    result = ModDataDir(modName);
    return result.c_str();
}

} // namespace Kuru::Storage
