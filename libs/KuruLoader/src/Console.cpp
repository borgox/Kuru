#include "Console.hpp"

#include "ModRegistry.hpp"
#include "Lifecycle.hpp"
#include "Storage.hpp"

#include <imgui.h>

#include <deque>
#include <filesystem>
#include <fstream>
#include <mutex>
#include <string>
#include <unordered_map>
#include <vector>

namespace Kuru::Console {
namespace {

// Matches Kuru::LogLevel: Trace=0, Info=1, Warning=2, Error=3.
constexpr int kLevelCommand = -1;  // console echo of a typed command

struct LogEntry {
    int level;
    std::string tag;
    std::string message;
};

std::mutex gMutex;
std::deque<LogEntry> gLog;
constexpr std::size_t kMaxLines = 4000;

struct Command {
    std::string help;
    CommandFn fn;
    void* user;
    void* owner;  // the mod that registered it (nullptr for host builtins)
};
std::unordered_map<std::string, Command> gCommands;

char gInput[512] = {};
bool gScrollToBottom = false;
bool gBuiltinsRegistered = false;

const char* LevelName(int level) {
    switch (level) {
    case 0: return "TRACE";
    case 2: return "WARN";
    case 3: return "ERROR";
    case kLevelCommand: return "CMD";
    default: return "INFO";
    }
}

ImVec4 LevelColor(int level) {
    switch (level) {
    case 0: return ImVec4(0.55f, 0.55f, 0.55f, 1.0f);          // trace: gray
    case 2: return ImVec4(0.95f, 0.80f, 0.30f, 1.0f);          // warn: yellow
    case 3: return ImVec4(0.95f, 0.35f, 0.40f, 1.0f);          // error: red
    case kLevelCommand: return ImVec4(0.55f, 0.75f, 0.95f, 1.0f); // command echo: blue
    default: return ImVec4(0.86f, 0.86f, 0.86f, 1.0f);         // info: near-white
    }
}

void WriteToFile(const LogEntry& entry) {
    static std::ofstream file = [] {
        const std::filesystem::path path = Kuru::Storage::KuruDir() / "kuru.log";
        std::error_code ec;
        if (std::filesystem::exists(path) && std::filesystem::file_size(path, ec) > 5 * 1024 * 1024) {
            std::filesystem::remove(path, ec);  // simple rotation: drop the old log if it's large
        }
        return std::ofstream(path, std::ios::app);
    }();
    if (file) {
        file << '[' << LevelName(entry.level) << "][" << entry.tag << "] " << entry.message << '\n';
        file.flush();
    }
}

// Assumes gMutex is held.
void PushLocked(int level, const std::string& tag, const std::string& message) {
    gLog.push_back({level, tag, message});
    while (gLog.size() > kMaxLines) {
        gLog.pop_front();
    }
    WriteToFile(gLog.back());
}

void RegisterBuiltins() {
    RegisterCommand("help", "list all commands", [](const char*, void*) {
        std::scoped_lock lock(gMutex);
        for (const auto& [name, cmd] : gCommands) {
            PushLocked(1, "Kuru", "  " + name + " - " + cmd.help);
        }
    }, nullptr);
    RegisterCommand("clear", "clear the console", [](const char*, void*) {
        std::scoped_lock lock(gMutex);
        gLog.clear();
    }, nullptr);
}

} // namespace

void AddLogTagged(int level, const char* tag, const char* message) {
    if (!message) {
        return;
    }
    std::scoped_lock lock(gMutex);
    PushLocked(level, tag ? tag : "", message);
}

void AddLog(const char* message) {
    AddLogTagged(1, "Kuru", message);  // Info, host tag
}

void RegisterCommand(const char* name, const char* help, const CommandFn fn, void* user) {
    if (Kuru::Lifecycle::stopping.load() || !name || !fn) {
        return;
    }
    void* owner = Kuru::ModRegistry::CurrentMod();
    std::scoped_lock lock(gMutex);
    gCommands[name] = {help ? help : "", fn, user, owner};
}

void RemoveCommandsForMod(const void* mod) {
    std::scoped_lock lock(gMutex);
    for (auto it = gCommands.begin(); it != gCommands.end();) {
        it = (it->second.owner == mod) ? gCommands.erase(it) : std::next(it);
    }
}

void Execute(const std::string& commandLine) {
    if (Kuru::Lifecycle::stopping.load()) return;
    gScrollToBottom = true;  // jump to the newest output at once, so the command's result is visible
    {
        std::scoped_lock lock(gMutex);
        PushLocked(kLevelCommand, "Kuru", "> " + commandLine);
    }
    const std::size_t space = commandLine.find(' ');
    const std::string name = commandLine.substr(0, space);
    const std::string args = (space == std::string::npos) ? std::string() : commandLine.substr(space + 1);

    Command command{};
    {
        std::scoped_lock lock(gMutex);
        const auto it = gCommands.find(name);
        if (it == gCommands.end()) {
            PushLocked(2, "Kuru", "unknown command: " + name + " (try 'help')");
            return;
        }
        command = it->second;  // copy so we don't hold the lock across the callback
    }
    command.fn(args.c_str(), command.user);
}

void Draw(bool* open) {
    if (!gBuiltinsRegistered) {
        RegisterBuiltins();
        gBuiltinsRegistered = true;
    }

    ImGui::SetNextWindowPos(ImVec2(520, 340), ImGuiCond_FirstUseEver);
    ImGui::SetNextWindowSize(ImVec2(700, 440), ImGuiCond_FirstUseEver);
    if (!ImGui::Begin("Kuru Console", open)) {
        ImGui::End();
        return;
    }

    static int minLevel = 0;
    static char tagFilter[64] = "";
    ImGui::TextDisabled("Level >=");
    ImGui::SameLine();
    ImGui::SetNextItemWidth(90.0f);
    ImGui::Combo("##level", &minLevel, "Trace\0Info\0Warn\0Error\0");
    ImGui::SameLine();
    ImGui::SetNextItemWidth(160.0f);
    ImGui::InputTextWithHint("##tag", "filter by mod tag", tagFilter, sizeof(tagFilter));
    ImGui::Separator();

    const float footer = ImGui::GetFrameHeightWithSpacing();
    ImGui::BeginChild("scroll", ImVec2(0, -footer), true, ImGuiWindowFlags_HorizontalScrollbar);
    {
        std::scoped_lock lock(gMutex);
        for (const LogEntry& entry : gLog) {
            if (entry.level != kLevelCommand && entry.level < minLevel) {
                continue;
            }
            if (tagFilter[0] != '\0' && entry.tag.find(tagFilter) == std::string::npos) {
                continue;
            }
            const ImVec4 color = LevelColor(entry.level);
            ImGui::TextColored(color, "[%s]", entry.tag.c_str());
            ImGui::SameLine();
            ImGui::TextColored(color, "%s", entry.message.c_str());
        }
    }
    // Auto-scroll to the newest line only while the user is already pinned to the bottom
    if (gScrollToBottom || ImGui::GetScrollY() >= ImGui::GetScrollMaxY()) {
        ImGui::SetScrollHereY(1.0f);
    }
    gScrollToBottom = false;
    ImGui::EndChild();

    ImGui::Separator();
    ImGui::SetNextItemWidth(-1.0f);
    if (ImGui::InputText("##input", gInput, sizeof(gInput), ImGuiInputTextFlags_EnterReturnsTrue)) {
        if (gInput[0] != '\0') {
            Execute(gInput);
            gInput[0] = '\0';
        }
        ImGui::SetKeyboardFocusHere(-1);
    }
    ImGui::End();
}

} // namespace Kuru::Console
