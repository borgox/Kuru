#pragma once
#include <cstddef>
#include <filesystem>

namespace Kuru::SdkGen {
struct BundleFile {
    const char* path;
    const unsigned char* data;
    size_t size;
};
extern const BundleFile templateFiles[];
extern const size_t templateFileCount;
// Throws on failure; reserves a new folder so existing mod sources are preserved.
std::filesystem::path PackageTemplate(const std::filesystem::path& generatedSdk);
}
