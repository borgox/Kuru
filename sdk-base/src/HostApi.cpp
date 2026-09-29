#include <Kuru/HostApi.hpp>
#include <Kuru/Log.hpp>

namespace Kuru {
namespace {
// Per-mod pointer to the host's API table, the host pushes the same table into all of them via KuruInitHostApi.
const HostApi* gHostApi = nullptr;
const char* gModTag = "mod";
}

void SetModTag(const char* tag) noexcept {
    gModTag = tag ? tag : "mod";
}

const char* GetModTag() noexcept {
    return gModTag;
}

void SetHostApi(const HostApi* api) noexcept {
    gHostApi = api;
    // Route this mod's Kuru::Log through the host so its output appears in the shared console,
    // carrying the level and this mod's tag.
    if (api && api->Log) {
        SetLogger([](LogLevel level, const char* message) {
            if (gHostApi && gHostApi->Log) {
                gHostApi->Log(static_cast<int>(level), gModTag, message);
            }
        });
    }
}

const HostApi* GetHostApi() noexcept {
    return gHostApi;
}

} // namespace Kuru
