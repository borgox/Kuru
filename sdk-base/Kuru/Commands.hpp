#pragma once

#include <Kuru/HostApi.hpp>

#include <functional>
#include <utility>

// Mod-facing console command API. Register a named command it runs when typed in the Kuru console (F3)

namespace Kuru::Commands {
namespace detail {

using Handler = std::function<void(const char* args)>;

inline void Thunk(const char* args, void* user) {
    (*static_cast<Handler*>(user))(args);
}

} // namespace detail

inline void Register(const char* name, const char* help, detail::Handler handler) {
    const HostApi* api = GetHostApi();
    if (!api || api->Version < 4 || !api->RegisterOwnedCommand) {
        return;
    }
    auto* holder = new detail::Handler(std::move(handler));
    api->RegisterOwnedCommand(name, help, &detail::Thunk, holder,
                             [](void* user) { delete static_cast<detail::Handler*>(user); });
}

} // namespace Kuru::Commands
