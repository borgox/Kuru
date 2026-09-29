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

// Enable/disable a mod live. Disabling calls OnUnload + removes the mod's hooks/commands; enabling re-runs OnLoad.
void SetEnabled(Entry& entry, bool enabled);

// Reload: destroy the instance and build a fresh one (resets all in-memory state), then OnLoad.
void Reload(Entry& entry);

} // namespace Kuru::ModRegistry
