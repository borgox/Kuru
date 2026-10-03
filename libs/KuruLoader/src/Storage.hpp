#pragma once

#include <filesystem>

// Persistent storage locations, resolved once. Exposed to mods via the HostApi (Kuru/Storage.hpp).

namespace Kuru::Storage {

const std::filesystem::path& KuruDir();               // %APPDATA%\Kuru\  (created)
const std::filesystem::path& GameDir();               // RL install dir (parent of RocketLeague.exe)
std::filesystem::path ModDataDir(const char* modName); // %APPDATA%\Kuru\data\<mod>\  (created)

// HostApi glue (host-owned strings)
const char* HostGetKuruDir();
const char* HostGetGameDir();
const char* HostGetModDataDir(const char* modName);

} // namespace Kuru::Storage
