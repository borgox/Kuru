#pragma once

// Global Variables (CVars): named, string-backed, persistent config saved to
// %APPDATA%\Kuru\config.cfg. Host-owned registry; mods use it via the HostApi (Kuru/Variables.hpp).
// Console-integrated: `set <name> <value>`, `get <name>`, `cvarlist`.

namespace Kuru::Variables {

void RegisterCVar(const char* name, const char* defaultValue, const char* description, void* owner);
// HostApi entry: tags the CVar with the currently-loading mod so it's removed when the mod unloads.
void HostRegisterCVar(const char* name, const char* defaultValue, const char* description);
const char* GetCVar(const char* name);   // host-owned c_str; copy immediately
void SetCVar(const char* name, const char* value);
void DeleteCVar(const char* name);       // remove it and drop it from the saved config
void RemoveCVarsForMod(void* owner);
void RegisterConsoleCommands();           // set/get/cvarlist
void Draw();                              // the Settings tab UI

} // namespace Kuru::Variables
