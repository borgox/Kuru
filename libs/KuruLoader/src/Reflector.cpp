#include "Reflector.hpp"

#include "PropertyView.hpp"

#include <Core/Globals.hpp>
#include <Core/ObjectModel.hpp>

#include <imgui.h>

#include <cctype>
#include <string>
#include <utility>
#include <vector>

namespace Kuru::Reflector {
namespace {

bool IsObjectAlive(Kuru::SDK::UObject* object) {
    if (!object) {
        return false;
    }
    auto* objects = Kuru::SDK::GetGlobalObjects();
    if (!objects) {
        return false;
    }
    for (Kuru::SDK::UObject* candidate : *objects) {
        if (candidate == object) {
            return true;
        }
    }
    return false;
}

void DrawProperties(Kuru::SDK::UObject* object) {
    Kuru::SDK::UClass* cls = object->GetClass();
    if (!cls) {
        ImGui::TextDisabled("no class");
        return;
    }
    auto* base = reinterpret_cast<unsigned char*>(object);
    const std::size_t size = static_cast<std::size_t>(cls->GetPropertiesSize());

    // Walk the class hierarchy; each struct's Children holds the properties DECLARED there.
    // Grouped under a collapsible header per class (most-derived expanded by default).
    int total = 0;
    bool firstGroup = true;
    for (Kuru::SDK::UStruct* s = cls; s; s = s->GetSuperStruct()) {
        std::vector<Kuru::SDK::UProperty*> props;
        for (Kuru::SDK::UField* field = s->GetChildren(); field; field = field->Next) {
            Kuru::SDK::UClass* fieldClass = field->GetClass();
            if (fieldClass && fieldClass->GetName().find("Property") != std::string::npos) {
                props.push_back(reinterpret_cast<Kuru::SDK::UProperty*>(field));
            }
        }
        if (props.empty()) {
            continue;
        }
        total += static_cast<int>(props.size());
        if (firstGroup) {
            ImGui::SetNextItemOpen(true, ImGuiCond_Once);
            firstGroup = false;
        }
        const std::string header = s->GetName() + "  (" + std::to_string(props.size()) + ")";
        if (ImGui::CollapsingHeader(header.c_str())) {
            for (Kuru::SDK::UProperty* prop : props) {
                Kuru::PropertyView::Draw(prop, base, size);  // reads/edits the LIVE object in place
            }
        }
    }
    if (total == 0) {
        ImGui::TextDisabled("no reflected properties");
    }
}

} // namespace

void Draw() {
    static char search[128] = "";
    static std::vector<std::pair<Kuru::SDK::UObject*, std::string>> matches;
    static Kuru::SDK::UObject* selected = nullptr;

    // Category filters
    static bool showInstances = true;
    static bool showClasses = true;
    static bool showFunctions = true;
    static bool showProperties = true;
    static bool showEnums = true;
    static bool showStructs = true;
    static bool caseInsensitive = false;

    auto* allObjects = Kuru::SDK::GetGlobalObjects();
    ImGui::Text("GObjects: %d total", allObjects ? allObjects->Num() : -1);
    ImGui::TextDisabled("Search live objects, select one to inspect/edit its properties.");
    bool doSearch = ImGui::InputTextWithHint("##search", "name filter, then Enter", search,
                                             sizeof(search), ImGuiInputTextFlags_EnterReturnsTrue);
    ImGui::SameLine();
    if (ImGui::Button("Search")) {
        doSearch = true;
    }

    ImGui::TextDisabled("Show:");
    ImGui::SameLine();
    bool f = false;
    f |= ImGui::Checkbox("Instances", &showInstances); ImGui::SameLine();
    f |= ImGui::Checkbox("Classes", &showClasses); ImGui::SameLine();
    f |= ImGui::Checkbox("Functions", &showFunctions); ImGui::SameLine();
    f |= ImGui::Checkbox("Properties", &showProperties); ImGui::SameLine();
    f |= ImGui::Checkbox("Enums", &showEnums); ImGui::SameLine();
    f |= ImGui::Checkbox("Structs", &showStructs); ImGui::SameLine();
    f |= ImGui::Checkbox("Aa", &caseInsensitive);
    if (ImGui::IsItemHovered()) {
        ImGui::SetTooltip("case-insensitive search");
    }
    if (f) {
        doSearch = true;
    }

    static bool initialized = false;
    if (!initialized) {
        doSearch = true;
        initialized = true;
    }
    if (doSearch) {
        matches.clear();
        // Resolve the category classes once.
        Kuru::SDK::UClass* cProp = Kuru::SDK::UProperty::StaticClass();
        Kuru::SDK::UClass* cFunc = Kuru::SDK::UFunction::StaticClass();
        Kuru::SDK::UClass* cEnum = Kuru::SDK::UEnum::StaticClass();
        Kuru::SDK::UClass* cClass = Kuru::SDK::UClass::StaticClass();
        Kuru::SDK::UClass* cStruct = Kuru::SDK::UStruct::StaticClass();
        std::string needle = search;
        if (caseInsensitive) {
            for (char& ch : needle) ch = static_cast<char>(std::tolower(static_cast<unsigned char>(ch)));
        }
        if (auto* objects = Kuru::SDK::GetGlobalObjects()) {
            for (Kuru::SDK::UObject* object : *objects) {
                if (!object) {
                    continue;
                }
                bool keep;
                if (cProp && object->IsA(cProp)) {
                    keep = showProperties;
                } else if (cFunc && object->IsA(cFunc)) {
                    keep = showFunctions;
                } else if (cEnum && object->IsA(cEnum)) {
                    keep = showEnums;
                } else if (cClass && object->IsA(cClass)) {
                    keep = showClasses;
                } else if (cStruct && object->IsA(cStruct)) {
                    keep = showStructs;
                } else {
                    keep = showInstances;
                }
                if (!keep) {
                    continue;
                }
                std::string full = object->GetFullName();
                bool match = needle.empty();
                if (!match) {
                    if (caseInsensitive) {
                        std::string lower = full;
                        for (char& ch : lower) ch = static_cast<char>(std::tolower(static_cast<unsigned char>(ch)));
                        match = lower.find(needle) != std::string::npos;
                    } else {
                        match = full.find(needle) != std::string::npos;
                    }
                }
                if (match) {
                    matches.emplace_back(object, std::move(full));
                    if (matches.size() >= 3000) {
                        break;  // cap the result set
                    }
                }
            }
        }
    }
    ImGui::Text("%d object(s)", static_cast<int>(matches.size()));
    ImGui::Separator();

    ImGui::BeginChild("objlist", ImVec2(340.0f, 0.0f), true);
    for (const auto& [object, name] : matches) {
        if (ImGui::Selectable(name.c_str(), selected == object)) {
            selected = object;
        }
    }
    ImGui::EndChild();

    ImGui::SameLine();
    ImGui::BeginChild("props", ImVec2(0.0f, 0.0f), true);
    if (!selected) {
        ImGui::TextDisabled("select an object on the left");
    } else if (!IsObjectAlive(selected)) {
        ImGui::TextDisabled("selected object is no longer alive");
        selected = nullptr;
    } else {
        ImGui::TextColored(ImVec4(0.96f, 0.30f, 0.50f, 1.0f), "%s", selected->GetFullName().c_str());
        ImGui::Separator();
        DrawProperties(selected);
    }
    ImGui::EndChild();
}

} // namespace Kuru::Reflector
