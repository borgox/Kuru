#include "Mods.hpp"

#include <windows.h>
#include <shellapi.h>

#include <algorithm>
#include <cwctype>

namespace Kuru::Injector {

namespace {

constexpr std::string_view kDisabledSuffix = ".disabled";

[[nodiscard]] bool EndsWithLower(const std::string& value, std::string_view suffix) {
    if (value.size() < suffix.size()) {
        return false;
    }
    return std::equal(suffix.rbegin(), suffix.rend(), value.rbegin(),
                      [](char a, char b) { return std::tolower(a) == std::tolower(b); });
}

} // namespace

std::filesystem::path ModsDirForLoader(const std::string& loaderDllPath) {
    if (loaderDllPath.empty()) {
        return {};
    }
    const std::filesystem::path loader(loaderDllPath);
    if (!loader.has_parent_path()) {
        return {};
    }
    return loader.parent_path() / "mods";
}

std::vector<ModEntry> ListMods(const std::filesystem::path& modsDir) {
    std::vector<ModEntry> mods;
    std::error_code ec;
    if (modsDir.empty() || !std::filesystem::is_directory(modsDir, ec)) {
        return mods;
    }

    for (const auto& entry : std::filesystem::directory_iterator(modsDir, ec)) {
        if (!entry.is_regular_file(ec)) {
            continue;
        }
        const std::string fileName = entry.path().filename().string();

        ModEntry mod;
        mod.path = entry.path();
        if (EndsWithLower(fileName, ".dll")) {
            mod.enabled = true;
            mod.name = fileName.substr(0, fileName.size() - 4);
        } else if (EndsWithLower(fileName, std::string(".dll") + std::string(kDisabledSuffix))) {
            mod.enabled = false;
            mod.name = fileName.substr(0, fileName.size() - 4 - kDisabledSuffix.size());
        } else {
            continue;  // not a mod file
        }
        mods.push_back(std::move(mod));
    }

    std::sort(mods.begin(), mods.end(), [](const ModEntry& a, const ModEntry& b) {
        return _stricmp(a.name.c_str(), b.name.c_str()) < 0;
    });
    return mods;
}

bool InstallMod(const std::filesystem::path& modsDir, const std::string& sourceFile, std::string& error) {
    if (modsDir.empty()) {
        error = "Set a valid mod loader path first";
        return false;
    }
    const std::filesystem::path source(sourceFile);
    std::string ext = source.extension().string();
    std::transform(ext.begin(), ext.end(), ext.begin(),
                   [](unsigned char c) { return static_cast<char>(std::tolower(c)); });
    if (ext != ".dll") {
        error = "Not a .dll: " + source.filename().string();
        return false;
    }

    std::error_code ec;
    std::filesystem::create_directories(modsDir, ec);
    const std::filesystem::path dest = modsDir / source.filename();
    std::filesystem::copy_file(source, dest, std::filesystem::copy_options::overwrite_existing, ec);
    if (ec) {
        error = "Could not copy " + source.filename().string() + ": " + ec.message();
        return false;
    }
    return true;
}

bool SetModEnabled(const ModEntry& mod, bool enabled, std::string& error) {
    if (mod.enabled == enabled) {
        return true;
    }
    std::filesystem::path target = mod.path;
    if (enabled) {
        // Drop the trailing .disabled suffix.
        std::string s = mod.path.string();
        target = s.substr(0, s.size() - kDisabledSuffix.size());
    } else {
        target = mod.path.string() + std::string(kDisabledSuffix);
    }

    std::error_code ec;
    std::filesystem::rename(mod.path, target, ec);
    if (ec) {
        error = "Could not " + std::string(enabled ? "enable" : "disable") + " " + mod.name + ": " +
                ec.message();
        return false;
    }
    return true;
}

bool RemoveMod(const ModEntry& mod, std::string& error) {
    std::error_code ec;
    std::filesystem::remove(mod.path, ec);
    if (ec) {
        error = "Could not remove " + mod.name + ": " + ec.message();
        return false;
    }
    return true;
}

void OpenInExplorer(const std::filesystem::path& dir) {
    if (dir.empty()) {
        return;
    }
    std::error_code ec;
    std::filesystem::create_directories(dir, ec);
    ShellExecuteW(nullptr, L"open", dir.wstring().c_str(), nullptr, nullptr, SW_SHOWNORMAL);
}

} // namespace Kuru::Injector
