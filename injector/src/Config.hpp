#pragma once

#include <climits>
#include <filesystem>
#include <string>

namespace Kuru::Injector {

// Persisted injector settings, stored as a small key=value file under %APPDATA%\Kuru.
struct Config {
    std::string loaderPath;
    std::string generatorPath;
    bool autoInject = false;
    int winX = INT_MIN;  // INT_MIN / 0 mean "unset"
    int winY = INT_MIN;
    int winW = 0;
    int winH = 0;
};

// %APPDATA%\Kuru\injector.ini (empty if APPDATA is unavailable).
[[nodiscard]] std::filesystem::path ConfigPath();

[[nodiscard]] Config LoadConfig();
void SaveConfig(const Config& config);

} // namespace Kuru::Injector
