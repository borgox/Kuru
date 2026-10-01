#include "Config.hpp"

#include <cstdlib>
#include <fstream>
#include <sstream>

namespace Kuru::Injector {

namespace {

[[nodiscard]] std::filesystem::path KuruAppDataDir() {
    const char* appData = std::getenv("APPDATA");
    if (!appData || *appData == '\0') {
        return {};
    }
    return std::filesystem::path(appData) / "Kuru";
}

[[nodiscard]] int ParseInt(const std::string& value, int fallback) {
    try {
        return std::stoi(value);
    } catch (const std::exception&) {
        return fallback;
    }
}

} // namespace

std::filesystem::path ConfigPath() {
    const std::filesystem::path dir = KuruAppDataDir();
    return dir.empty() ? std::filesystem::path{} : dir / "injector.ini";
}

Config LoadConfig() {
    Config config;
    const std::filesystem::path path = ConfigPath();
    if (path.empty()) {
        return config;
    }

    std::ifstream in(path);
    std::string line;
    while (std::getline(in, line)) {
        const auto eq = line.find('=');
        if (eq == std::string::npos) {
            continue;
        }
        const std::string key = line.substr(0, eq);
        const std::string value = line.substr(eq + 1);
        if (key == "loader") {
            config.loaderPath = value;
        } else if (key == "generator") {
            config.generatorPath = value;
        } else if (key == "auto_inject") {
            config.autoInject = value == "1";
        } else if (key == "win_x") {
            config.winX = ParseInt(value, config.winX);
        } else if (key == "win_y") {
            config.winY = ParseInt(value, config.winY);
        } else if (key == "win_w") {
            config.winW = ParseInt(value, config.winW);
        } else if (key == "win_h") {
            config.winH = ParseInt(value, config.winH);
        }
    }
    return config;
}

void SaveConfig(const Config& config) {
    const std::filesystem::path path = ConfigPath();
    if (path.empty()) {
        return;
    }
    std::error_code ec;
    std::filesystem::create_directories(path.parent_path(), ec);

    std::ofstream out(path, std::ios::trunc);
    if (!out) {
        return;
    }
    out << "loader=" << config.loaderPath << '\n';
    out << "generator=" << config.generatorPath << '\n';
    out << "auto_inject=" << (config.autoInject ? "1" : "0") << '\n';
    out << "win_x=" << config.winX << '\n';
    out << "win_y=" << config.winY << '\n';
    out << "win_w=" << config.winW << '\n';
    out << "win_h=" << config.winH << '\n';
}

} // namespace Kuru::Injector
