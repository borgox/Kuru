#pragma once

#include <Kuru/HostApi.hpp>

#include <cstdlib>
#include <string>

/* Mod-facing global variables (CVars): named, persistent (%APPDATA%\Kuru\config.cfg), settable
 * from the console (set/get). Register once (e.g. in OnLoad), then Get/Set by name. Name your CVars uniquely
 */

namespace Kuru::Variables {

inline void Register(const char* name, const char* defaultValue, const char* description = "") {
    const HostApi* api = GetHostApi();
    if (api && api->RegisterCVar) {
        api->RegisterCVar(name, defaultValue, description);
    }
}

inline std::string GetString(const char* name) {
    const HostApi* api = GetHostApi();
    const char* p = api && api->GetCVar ? api->GetCVar(name) : "";
    return p ? std::string(p) : std::string();
}

inline int GetInt(const char* name) {
    return std::atoi(GetString(name).c_str());
}

inline float GetFloat(const char* name) {
    return static_cast<float>(std::atof(GetString(name).c_str()));
}

inline bool GetBool(const char* name) {
    const std::string v = GetString(name);
    return v == "1" || v == "true" || v == "True";
}

inline void Set(const char* name, const char* value) {
    const HostApi* api = GetHostApi();
    if (api && api->SetCVar) {
        api->SetCVar(name, value);
    }
}

inline void Set(const char* name, const std::string& value) { Set(name, value.c_str()); }
inline void Set(const char* name, int value) { Set(name, std::to_string(value)); }
inline void Set(const char* name, float value) { Set(name, std::to_string(value)); }
inline void Set(const char* name, bool value) { Set(name, value ? "1" : "0"); }

} // namespace Kuru::Variables
