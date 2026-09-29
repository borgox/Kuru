#include "ModRegistry.hpp"

#include "Console.hpp"
#include "HookEngine.hpp"
#include "Variables.hpp"

#include <Kuru/Mod.hpp>

namespace Kuru::ModRegistry {
namespace {
Kuru::IMod* gCurrentMod = nullptr;
}

std::vector<Entry>& Entries() {
    static std::vector<Entry> entries;
    return entries;
}

Kuru::IMod* CurrentMod() {
    return gCurrentMod;
}

void SetCurrentMod(Kuru::IMod* mod) {
    gCurrentMod = mod;
}

void SetEnabled(Entry& entry, bool enabled) {
    if (entry.enabled == enabled || !entry.instance) {
        entry.enabled = enabled;
        return;
    }
    entry.enabled = enabled;
    SetCurrentMod(entry.instance);
    if (enabled) {
        entry.instance->OnLoad();  // re-register hooks/commands/GUI
    } else {
        entry.instance->OnUnload();                                   // the mod's own teardown
        Kuru::HookEngine::RemoveHooksForMod(entry.instance);          // framework default teardown
        Kuru::Console::RemoveCommandsForMod(entry.instance);
        Kuru::Variables::RemoveCVarsForMod(entry.instance);
    }
    SetCurrentMod(nullptr);
}

void Reload(Entry& entry) {
    if (!entry.instance || !entry.create) {
        return;
    }
    // Tear down the current instance (its own OnUnload + framework cleanup of hooks/commands).
    SetCurrentMod(entry.instance);
    entry.instance->OnUnload();
    Kuru::HookEngine::RemoveHooksForMod(entry.instance);
    Kuru::Console::RemoveCommandsForMod(entry.instance);
    Kuru::Variables::RemoveCVarsForMod(entry.instance);
    if (entry.destroy) {
        entry.destroy(entry.instance);
    }
    // Build a fresh instance from scratch — all member state resets to defaults.
    entry.instance = entry.create();
    entry.enabled = true;
    SetCurrentMod(entry.instance);
    if (entry.instance) {
        entry.instance->OnLoad();
    }
    SetCurrentMod(nullptr);
}

} // namespace Kuru::ModRegistry
