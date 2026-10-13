#include "TemplateBundle.hpp"
#include <fstream>
#include <stdexcept>

namespace Kuru::SdkGen {
std::filesystem::path PackageTemplate(const std::filesystem::path& generatedSdk) {
    if (!std::filesystem::is_regular_file(generatedSdk / "KuruSDK.hpp"))
        throw std::runtime_error("Generated SDK headers are missing");
    std::filesystem::path destination;
    for (unsigned i = 1;; ++i) {
        destination = generatedSdk.parent_path() /
            (i == 1 ? "mod-template" : "mod-template-" + std::to_string(i));
        if (std::filesystem::create_directory(destination)) break;
    }
    for (size_t i = 0; i < templateFileCount; ++i) {
        const auto& file = templateFiles[i];
        const auto path = destination / file.path;
        std::filesystem::create_directories(path.parent_path());
        std::ofstream stream;
        stream.exceptions(std::ios::failbit | std::ios::badbit);
        stream.open(path, std::ios::binary);
        stream.write(reinterpret_cast<const char*>(file.data), static_cast<std::streamsize>(file.size));
        stream.close();
    }
    std::filesystem::copy(generatedSdk, destination / "generated-sdk",
                          std::filesystem::copy_options::recursive);
    return destination;
}
}
