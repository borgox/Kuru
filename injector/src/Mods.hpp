#pragma once

#include <filesystem>
#include <string>
#include <vector>

namespace Kuru::Injector {

// A mod is a .dll in the loader's mods folder. It is "disabled" by giving it a .disabled suffix so the loader (which only scans *.dll) skips it, without deleting the file.
struct ModEntry {
    std::string name;               // display name, no extension
    std::filesystem::path path;     // full path to the file on disk
    bool enabled = true;
};

// The folder the loader scans: <loader dll directory>\mods. Empty if the loader path has no parent.
[[nodiscard]] std::filesystem::path ModsDirForLoader(const std::string& loaderDllPath);

// Enumerate mods in the folder (both enabled *.dll and disabled *.dll.disabled), sorted by name.
[[nodiscard]] std::vector<ModEntry> ListMods(const std::filesystem::path& modsDir);

// Copy a .dll into the mods folder (creating it if needed), overwriting a same-named mod. Returns false and fills error on failure or if the source is not a .dll.
[[nodiscard]] bool InstallMod(const std::filesystem::path& modsDir, const std::string& sourceFile,
                              std::string& error);

// Enable/disable by toggling the .disabled suffix.
[[nodiscard]] bool SetModEnabled(const ModEntry& mod, bool enabled, std::string& error);

// Delete the mod file.
[[nodiscard]] bool RemoveMod(const ModEntry& mod, std::string& error);

// Open a folder in Explorer (creating it first so the window always opens).
void OpenInExplorer(const std::filesystem::path& dir);

} // namespace Kuru::Injector
