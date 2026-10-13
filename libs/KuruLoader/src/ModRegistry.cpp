#include "ModRegistry.hpp"

#include "Console.hpp"
#include "HookEngine.hpp"
#include "Variables.hpp"
#include "ThreadBridge.hpp"

#include <Kuru/Mod.hpp>
#include <exception>

namespace Kuru::ModRegistry {
namespace {
thread_local Kuru::IMod* gCurrentMod = nullptr;
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

void RemoveRegistrations(Kuru::IMod* mod) {
    if (!mod) return;
    Kuru::ThreadBridge::CancelForMod(mod);
    Kuru::HookEngine::RemoveHooksForMod(mod);  // includes event listeners
    Kuru::Console::RemoveCommandsForMod(mod);
    Kuru::Variables::RemoveCVarsForMod(mod);
}

namespace {
void Enable(Entry& entry) {
    entry.enabled = false;
    if (!entry.instance) {
        Console::AddLog(("mod enable failed (no instance): " + entry.name).c_str());
        return;
    }
    ScopedCurrentMod owner(entry.instance);
    ThreadBridge::BeginForMod(entry.instance);
    try {
        if (entry.instance->OnLoad()) {
            entry.enabled = true;
            return;
        }
        Console::AddLog(("mod OnLoad returned false; kept disabled: " + entry.name).c_str());
    } catch (const std::exception& error) {
        Console::AddLog(("mod OnLoad threw; kept disabled: " + entry.name + ": " + error.what()).c_str());
    } catch (...) {
        Console::AddLog(("mod OnLoad threw; kept disabled: " + entry.name).c_str());
    }
    // A partial load does not receive OnUnload; undo host registrations and queued work.
    RemoveRegistrations(entry.instance);
}
}

void Deactivate(Entry& entry) {
    const bool wasEnabled = entry.enabled;
    entry.enabled = false;
    if (!entry.instance) return;
    ScopedCurrentMod owner(entry.instance);
    ThreadBridge::CancelForMod(entry.instance);
    if (wasEnabled) {
        try {
            entry.instance->OnUnload();
        } catch (const std::exception& error) {
            Console::AddLog(("mod OnUnload threw; continuing cleanup: " + entry.name + ": " + error.what()).c_str());
        } catch (...) {
            Console::AddLog(("mod OnUnload threw; continuing cleanup: " + entry.name).c_str());
        }
    }
    RemoveRegistrations(entry.instance);
}

void SetEnabled(Entry& entry, bool enabled) {
    if (!entry.instance) {
        entry.enabled = false;
        return;
    }
    if (entry.enabled == enabled) return;
    if (enabled) {
        Enable(entry);
    } else {
        Deactivate(entry);
    }
}

void Reload(Entry& entry) {
    if (!entry.create || !entry.destroy) return;
    Deactivate(entry);
    if (entry.instance) {
        ScopedCurrentMod owner(entry.instance);
        auto* old = entry.instance;
        entry.instance = nullptr;
        entry.destroy(old);
    }
    try {
        entry.instance = entry.create();
    } catch (const std::exception& error) {
        Console::AddLog(("mod creation failed; kept disabled: " + entry.name + ": " + error.what()).c_str());
        return;
    } catch (...) {
        Console::AddLog(("mod creation failed; kept disabled: " + entry.name).c_str());
        return;
    }
    Enable(entry);
}

} // namespace Kuru::ModRegistry
