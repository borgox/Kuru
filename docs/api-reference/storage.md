# Kuru storage paths

Paths for reading and writing files. These are in the `Kuru` namespace directly (`Kuru::GetKuruDir`).

## Functions

### `GetKuruDir() -> std::string`

Kuru's data directory (`%APPDATA%\Kuru`). The log file lives here; `%APPDATA%` already includes Roaming.

### `GetGameDir() -> std::string`

The directory containing the running `RocketLeague.exe`, normally `Binaries\Win64`, not the installation root.

### `GetModDataDir(const char* modName) -> std::string`

A folder under `%APPDATA%\Kuru\data\<modName>`, created on request. Use your mod's simple name.
The host does not sanitize path components, so do not pass user-supplied paths as the name. The wrapper returns a string you own.

If APPDATA is missing or too long for the host's buffer, the data-directory fallback is `.\Kuru`.
`GetGameDir` returns an empty string if the executable path cannot be obtained.
