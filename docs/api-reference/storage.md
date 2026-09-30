# Kuru::Storage

Paths for reading and writing files. These are in the `Kuru` namespace directly (`Kuru::GetKuruDir`).

## Functions

`GetKuruDir() -> std::string`
Kuru's data directory (`%AppData%/Roaming/Kuru`). The log file lives here.

`GetGameDir() -> std::string`
Rocket League's install directory.

`GetModDataDir(const char* modName) -> std::string`
A per-mod folder under Kuru's data directory for your mod to store files in.
