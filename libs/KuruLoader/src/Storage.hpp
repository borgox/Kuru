#pragma once

#include <string>

// Persistent storage locations, resolved once. Exposed to mods via the HostApi (Kuru/Storage.hpp).

namespace Kuru::Storage {

const std::string& KuruDir();               // %APPDATA%\Kuru\  (created)
const std::string& GameDir();               // RL install dir (parent of RocketLeague.exe)
std::string ModDataDir(const char* modName); // %APPDATA%\Kuru\data\<mod>\  (created)

// HostApi glue (host-owned strings)
const char* HostGetKuruDir();
const char* HostGetGameDir();
const char* HostGetModDataDir(const char* modName);

} // namespace Kuru::Storage
