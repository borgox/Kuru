#include "Scanner.hpp"

#include "PropertyView.hpp"

#include <Core/Globals.hpp>
#include <Core/ObjectModel.hpp>

#include <imgui.h>
#include <windows.h>

#include <atomic>
#include <cstdint>
#include <cstdio>
#include <cstring>
#include <deque>
#include <fstream>
#include <mutex>
#include <string>
#include <vector>

namespace Kuru::Scanner {
namespace {

// UE3 property flags.
constexpr std::uint64_t CPF_Parm = 0x80;
constexpr std::uint64_t CPF_OutParm = 0x100;
constexpr std::uint64_t CPF_ReturnParm = 0x400;

constexpr std::size_t kMaxRecords = 400;
constexpr std::uint16_t kMaxParamCopy = 2048;

struct CallRecord {
    Kuru::SDK::UFunction* function = nullptr;
    Kuru::SDK::UObject* caller = nullptr;   // for resend / alive check (may become stale)
    std::string functionName;
    std::string callerName;
    std::vector<std::uint8_t> params;       // captured copy of the param block
    unsigned long long sequence = 0;
};

std::atomic<bool> gEnabled{false};
std::mutex gMutex;
std::deque<CallRecord> gIncoming;   // game thread appends here (under gMutex)
std::deque<CallRecord> gDisplay;    // render thread ONLY
unsigned long long gSequence = 0;

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

// The parameter properties of a function, in declaration order.
std::vector<Kuru::SDK::UProperty*> ParamProperties(Kuru::SDK::UFunction* function) {
    std::vector<Kuru::SDK::UProperty*> params;
    if (!function) {
        return params;
    }
    for (Kuru::SDK::UField* field = function->Children; field; field = field->Next) {
        auto* prop = reinterpret_cast<Kuru::SDK::UProperty*>(field);
        if (prop->HasAnyPropertyFlags(CPF_Parm)) {
            params.push_back(prop);
        }
    }
    return params;
}

void ExportToFile() {
    const char* appData = std::getenv("APPDATA");
    const std::string path = (appData ? std::string(appData) + "\\Kuru" : std::string(".")) + "\\scan_export.txt";
    std::ofstream out(path, std::ios::trunc);
    if (!out) {
        return;
    }
    // Called from Draw() on the render thread; gDisplay is render-thread-only, so no lock needed.
    out << "Kuru Function Scanner export (" << gDisplay.size() << " calls, newest first)\n\n";
    for (const CallRecord& record : gDisplay) {
        out << "#" << record.sequence << "  " << record.functionName
            << "  caller=" << record.callerName << "\n";
        for (Kuru::SDK::UProperty* prop : ParamProperties(record.function)) {
            const std::uint32_t offset = static_cast<std::uint32_t>(prop->Offset);
            out << "    " << prop->GetName() << " (" << Kuru::PropertyView::KindOf(prop) << ") @0x"
                << std::hex << offset << std::dec << " =";
            const std::uint32_t end = offset + static_cast<std::uint32_t>(prop->ElementSize);
            if (end <= record.params.size()) {
                char hex[8];
                for (std::uint32_t i = offset; i < end && i < offset + 16; ++i) {
                    std::snprintf(hex, sizeof(hex), " %02X", record.params[i]);
                    out << hex;
                }
            }
            out << "\n";
        }
        out << "\n";
    }
}

} // namespace

bool IsEnabled() {
    return gEnabled.load(std::memory_order_relaxed);
}

void SetEnabled(bool enabled) {
    gEnabled.store(enabled, std::memory_order_relaxed);
}

void Record(Kuru::SDK::UObject* caller, Kuru::SDK::UFunction* function, void* params) {
    if (!gEnabled.load(std::memory_order_relaxed) || !function) {
        return;
    }
    CallRecord record;
    record.function = function;
    record.caller = caller;
    record.functionName = function->GetPathName();
    record.callerName = caller ? caller->GetName() : std::string("<null>");
    const std::uint16_t size = function->ParmsSize < kMaxParamCopy ? function->ParmsSize : kMaxParamCopy;
    if (params && size > 0) {
        record.params.assign(static_cast<std::uint8_t*>(params), static_cast<std::uint8_t*>(params) + size);
    }

    std::scoped_lock lock(gMutex);
    record.sequence = ++gSequence;
    gIncoming.push_back(std::move(record));
    // Cap even if the UI isn't draining (e.g. Functions tab closed) so this can't grow unbounded.
    while (gIncoming.size() > kMaxRecords) {
        gIncoming.pop_front();
    }
}

void Draw() {
    bool enabled = IsEnabled();
    if (ImGui::Checkbox("Scan ProcessEvent", &enabled)) {
        SetEnabled(enabled);
    }
    ImGui::SameLine();
    if (ImGui::Button("Clear")) {
        std::scoped_lock lock(gMutex);
        gIncoming.clear();
        gDisplay.clear();
    }
    ImGui::SameLine();
    if (ImGui::Button("Export to file")) {
        ExportToFile();
    }
    static char filter[128] = "";
    static bool paused = false;
    static bool hideNoParams = false;
    ImGui::InputTextWithHint("##filter", "filter by function name", filter, sizeof(filter));
    ImGui::TextDisabled("Settings:");
    ImGui::SameLine();
    ImGui::Checkbox("Pause", &paused);
    if (ImGui::IsItemHovered()) {
        ImGui::SetTooltip("freeze the list so new calls don't push it away (capture keeps running)");
    }
    ImGui::SameLine();
    ImGui::Checkbox("Hide param-less calls", &hideNoParams);
    ImGui::Separator();

    if (!paused) {
        std::scoped_lock lock(gMutex);
        while (!gIncoming.empty()) {
            gDisplay.push_front(std::move(gIncoming.front()));
            gIncoming.pop_front();
        }
    }
    while (gDisplay.size() > kMaxRecords) {
        gDisplay.pop_back();
    }

    ImGui::BeginChild("calls");
    for (CallRecord& record : gDisplay) {
        if (filter[0] != '\0' && record.functionName.find(filter) == std::string::npos) {
            continue;
        }
        if (hideNoParams && ParamProperties(record.function).empty()) {
            continue;
        }
        ImGui::PushID(static_cast<int>(record.sequence));
        char header[320];
        std::snprintf(header, sizeof(header), "%s   (%s)###call", record.functionName.c_str(),
                      record.callerName.c_str());
        if (ImGui::TreeNode(header)) {
            std::vector<Kuru::SDK::UProperty*> params = ParamProperties(record.function);
            if (params.empty()) {
                ImGui::TextDisabled("no parameters");
            }
            bool anyEdit = false;
            for (Kuru::SDK::UProperty* prop : params) {
                anyEdit |= Kuru::PropertyView::Draw(prop, record.params.data(), record.params.size());
            }
            (void)anyEdit;

            // Resend: only offered when there is a captured caller to target.
            if (record.caller) {
                if (ImGui::Button("Resend call")) {
                    if (IsObjectAlive(record.caller)) {
                        // Ensure the buffer is at least the full param block ProcessEvent will read.
                        if (record.params.size() < record.function->ParmsSize) {
                            record.params.resize(record.function->ParmsSize, 0);
                        }
                        try {
                            record.caller->ProcessEvent(record.function, record.params.data());
                        } catch (...) {
                        }
                    }
                }
                ImGui::SameLine();
                ImGui::TextDisabled("re-invokes with the edited params above");
            }
            ImGui::TreePop();
        }
        ImGui::PopID();
    }
    ImGui::EndChild();
}

} // namespace Kuru::Scanner
