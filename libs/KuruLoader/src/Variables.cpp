#include "Variables.hpp"

#include "Console.hpp"
#include "ModRegistry.hpp"
#include "Storage.hpp"

#include <imgui.h>

#include <filesystem>
#include <fstream>
#include <map>
#include <mutex>
#include <string>

namespace Kuru::Variables {
namespace {

struct CVar {
    std::string value;
    std::string defaultValue;
    std::string description;
    void* owner;
};

std::mutex gMutex;
std::map<std::string, CVar> gCVars;          // ordered for stable listing
std::map<std::string, std::string> gSaved;   // latest values, including inactive mods
bool gLoaded = false;

std::filesystem::path ConfigPath() {
    return Kuru::Storage::KuruDir() / "config.cfg";
}

void LoadLocked() {
    if (gLoaded) {
        return;
    }
    gLoaded = true;
    std::ifstream in(ConfigPath());
    std::string line;
    while (std::getline(in, line)) {
        const std::size_t eq = line.find('=');
        if (eq != std::string::npos) {
            gSaved[line.substr(0, eq)] = line.substr(eq + 1);
        }
    }
}

void SaveLocked() {
    std::ofstream out(ConfigPath(), std::ios::trunc);
    for (const auto& [name, value] : gSaved) {
        out << name << '=' << value << '\n';
    }
}

} // namespace

void RegisterCVar(const char* name, const char* defaultValue, const char* description, void* owner) {
    if (!name) {
        return;
    }
    std::scoped_lock lock(gMutex);
    LoadLocked();
    if (gCVars.count(name)) {
        return;  // already registered this session
    }
    std::string value = defaultValue ? defaultValue : "";
    if (const auto it = gSaved.find(name); it != gSaved.end()) {
        value = it->second;  // restore persisted value
    }
    gCVars[name] = {value, defaultValue ? defaultValue : "", description ? description : "", owner};
    gSaved[name] = value;
}

void HostRegisterCVar(const char* name, const char* defaultValue, const char* description) {
    RegisterCVar(name, defaultValue, description, Kuru::ModRegistry::CurrentMod());
}

const char* GetCVar(const char* name) {
    if (!name) {
        return "";
    }
    std::scoped_lock lock(gMutex);
    const auto it = gCVars.find(name);
    return it != gCVars.end() ? it->second.value.c_str() : "";
}

void SetCVar(const char* name, const char* value) {
    if (!name) {
        return;
    }
    std::scoped_lock lock(gMutex);
    LoadLocked();
    const auto it = gCVars.find(name);
    if (it != gCVars.end()) {
        it->second.value = value ? value : "";
    } else {
        gCVars[name] = {value ? value : "", "", "", nullptr};  // ad-hoc (e.g. set from console)
    }
    gSaved[name] = value ? value : "";
    SaveLocked();
}

void DeleteCVar(const char* name) {
    if (!name) {
        return;
    }
    std::scoped_lock lock(gMutex);
    LoadLocked();
    gCVars.erase(name);
    gSaved.erase(name);
    SaveLocked();
}

void RemoveCVarsForMod(void* owner) {
    std::scoped_lock lock(gMutex);
    for (auto it = gCVars.begin(); it != gCVars.end();) {
        it = (it->second.owner == owner) ? gCVars.erase(it) : std::next(it);
    }
}

void RegisterConsoleCommands() {
    Kuru::Console::RegisterCommand("set", "set <cvar> <value>", [](const char* args, void*) {
        std::string a = args ? args : "";
        const std::size_t sp = a.find(' ');
        if (sp == std::string::npos) {
            Kuru::Console::AddLog("usage: set <cvar> <value>");
            return;
        }
        SetCVar(a.substr(0, sp).c_str(), a.substr(sp + 1).c_str());
    }, nullptr);
    Kuru::Console::RegisterCommand("get", "get <cvar>", [](const char* args, void*) {
        Kuru::Console::AddLog((std::string(args ? args : "") + " = " + GetCVar(args)).c_str());
    }, nullptr);
    Kuru::Console::RegisterCommand("unset", "unset <cvar>  (delete it)", [](const char* args, void*) {
        DeleteCVar(args);
    }, nullptr);
    Kuru::Console::RegisterCommand("cvarlist", "list all cvars", [](const char*, void*) {
        std::scoped_lock lock(gMutex);
        for (const auto& [name, cvar] : gCVars) {
            Kuru::Console::AddLog(("  " + name + " = " + cvar.value +
                                   (cvar.description.empty() ? "" : "   // " + cvar.description)).c_str());
        }
    }, nullptr);
}

void Draw() {
    ImGui::TextDisabled("Named persistent variables, saved to %%APPDATA%%\\Kuru\\config.cfg");
    ImGui::Separator();
    ImGui::BeginChild("cvars");
    std::scoped_lock lock(gMutex);
    bool changed = false;
    std::string toDelete;
    for (auto& [name, cvar] : gCVars) {
        ImGui::PushID(name.c_str());
        if (ImGui::SmallButton("x")) {
            toDelete = name;
        }
        if (ImGui::IsItemHovered()) {
            ImGui::SetTooltip("delete this variable");
        }
        ImGui::SameLine();
        char buf[256];
        std::snprintf(buf, sizeof(buf), "%s", cvar.value.c_str());
        ImGui::SetNextItemWidth(220.0f);
        if (ImGui::InputText("##v", buf, sizeof(buf), ImGuiInputTextFlags_EnterReturnsTrue)) {
            cvar.value = buf;
            gSaved[name] = cvar.value;
            changed = true;
        }
        ImGui::SameLine();
        ImGui::Text("%s", name.c_str());
        if (!cvar.description.empty() && ImGui::IsItemHovered()) {
            ImGui::SetTooltip("%s", cvar.description.c_str());
        }
        ImGui::PopID();
    }
    if (!toDelete.empty()) {
        gCVars.erase(toDelete);
        gSaved.erase(toDelete);
        changed = true;
    }
    if (changed) {
        SaveLocked();
    }
    ImGui::EndChild();
}

} // namespace Kuru::Variables
