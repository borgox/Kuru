#include "PropertyView.hpp"

#include <Core/ObjectModel.hpp>

#include <imgui.h>

#include <cstdint>
#include <cstdio>
#include <string>

namespace Kuru::PropertyView {

const char* KindOf(Kuru::SDK::UProperty* prop) {
    static std::string kind;
    if (Kuru::SDK::UClass* cls = prop->GetClass()) {
        kind = cls->GetName();
        return kind.c_str();
    }
    return "Property";
}

bool Draw(Kuru::SDK::UProperty* prop, unsigned char* base, std::size_t size) {
    const std::string name = prop->GetName();
    const std::string kind = KindOf(prop);
    const std::uint32_t offset = static_cast<std::uint32_t>(prop->Offset);
    const bool inBounds = (offset + static_cast<std::uint32_t>(prop->ElementSize)) <= size;

    ImGui::PushID(prop);
    ImGui::TextColored(ImVec4(0.96f, 0.30f, 0.50f, 1.0f), "%s", name.c_str());
    ImGui::SameLine();
    ImGui::TextDisabled("(%s)", kind.c_str());
    ImGui::SameLine();

    bool edited = false;
    if (!inBounds) {
        ImGui::TextDisabled("<out of range>");
        ImGui::PopID();
        return false;
    }
    unsigned char* p = base + offset;

    if (kind == "IntProperty") {
        edited = ImGui::InputInt("##v", reinterpret_cast<int*>(p));
    } else if (kind == "FloatProperty") {
        edited = ImGui::InputFloat("##v", reinterpret_cast<float*>(p));
    } else if (kind == "ByteProperty") {
        int value = *p;
        if (ImGui::InputInt("##v", &value)) {
            *p = static_cast<unsigned char>(value & 0xFF);
            edited = true;
        }
    } else if (kind == "BoolProperty") {
        std::uint64_t mask = reinterpret_cast<Kuru::SDK::UBoolProperty*>(prop)->BitMask;
        std::uint32_t& dword = *reinterpret_cast<std::uint32_t*>(p);
        bool value = (dword & static_cast<std::uint32_t>(mask)) != 0;
        if (ImGui::Checkbox("##v", &value)) {
            if (value) {
                dword |= static_cast<std::uint32_t>(mask);
            } else {
                dword &= ~static_cast<std::uint32_t>(mask);
            }
            edited = true;
        }
    } else if (kind == "ObjectProperty" || kind == "ComponentProperty" || kind == "ClassProperty") {
        auto* obj = *reinterpret_cast<Kuru::SDK::UObject**>(p);
        ImGui::Text("%s", obj ? obj->GetName().c_str() : "None");
    } else {
        char hex[8] = {};
        const std::uint32_t n = prop->ElementSize > 8 ? 8 : static_cast<std::uint32_t>(prop->ElementSize);
        std::string dump;
        for (std::uint32_t i = 0; i < n; ++i) {
            std::snprintf(hex, sizeof(hex), "%02X ", p[i]);
            dump += hex;
        }
        ImGui::TextDisabled("%s", dump.empty() ? "<no data>" : dump.c_str());
    }
    ImGui::PopID();
    return edited;
}

} // namespace Kuru::PropertyView
