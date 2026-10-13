#pragma once

#include <string>
#include <vector>

namespace Kuru {
class IMod;
}

// The single list of loaded mods, shared by the loader (which populates + drives it) and the GUI
// (mod-manager page). Each entry carries display info and a live enabled flag.

namespace Kuru::ModRegistry {

struct Entry {
    Kuru::IMod* instance = nullptr;
    void* dll = nullptr;               // HMODULE
    void (*destroy)(Kuru::IMod*) = nullptr;
    Kuru::IMod* (*create)() = nullptr;  // to rebuild a fresh instance on Reload
    std::string name;
    std::string author;
    std::string version;
    std::wstring path;
    bool enabled = true;               // enable/disable = pause (keeps instance + state)
};

std::vector<Entry>& Entries();

// The mod whose OnLoad/OnUnload is currently running. Hooks/commands registered during that
// window are tagged with it, so the framework can remove them when the mod is disabled.
Kuru::IMod* CurrentMod();
void SetCurrentMod(Kuru::IMod* mod);

// Preserve ownership across nested callbacks and keep different threads independent.
class ScopedCurrentMod {
public:
    explicit ScopedCurrentMod(Kuru::IMod* mod) : previous_(CurrentMod()) { SetCurrentMod(mod); }
    ~ScopedCurrentMod() { SetCurrentMod(previous_); }
    ScopedCurrentMod(const ScopedCurrentMod&) = delete;
    ScopedCurrentMod& operator=(const ScopedCurrentMod&) = delete;
private:
    Kuru::IMod* previous_;
};

// Remove host registrations without calling mod code. Safe after a partial OnLoad.
void RemoveRegistrations(Kuru::IMod* mod);

// Disable, run OnUnload if previously enabled, and clean up even if OnUnload throws.
// Shared by disable, reload, and eject. Keeps the instance alive.
void Deactivate(Entry& entry);

// Enable/disable a mod live. Failed OnLoad attempts are cleaned up and stay disabled.
void SetEnabled(Entry& entry, bool enabled);

// Reload: recreate the instance, then OnLoad. Failed creation/loading leaves it disabled.
void Reload(Entry& entry);

} // namespace Kuru::ModRegistry
