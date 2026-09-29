#pragma once

#include <Kuru/HostApi.hpp>

#include <string>

// Mod-facing persistent storage locations. Use these instead of hardcoding paths.

namespace Kuru {

inline std::string GetKuruDir() {
    const HostApi* api = GetHostApi();
    const char* p = api && api->GetKuruDir ? api->GetKuruDir() : "";
    return p ? std::string(p) : std::string();
}

inline std::string GetGameDir() {
    const HostApi* api = GetHostApi();
    const char* p = api && api->GetGameDir ? api->GetGameDir() : "";
    return p ? std::string(p) : std::string();
}

// Per-mod data directory (created on demand): %APPDATA%\Kuru\data\<modName>\.
inline std::string GetModDataDir(const char* modName) {
    const HostApi* api = GetHostApi();
    const char* p = api && api->GetModDataDir ? api->GetModDataDir(modName) : "";
    return p ? std::string(p) : std::string();
}

} // namespace Kuru
