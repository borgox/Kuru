#pragma once

#include <string>

namespace Kuru::Console {

using CommandFn = void (*)(const char* args, void* user);

void AddLog(const char* message);                                      // convenience: Info / "Kuru"
void AddLogTagged(int level, const char* tag, const char* message);    // HostApi.Log target
void RegisterCommand(const char* name, const char* help, CommandFn fn, void* user);
void RemoveCommandsForMod(const void* mod);
void Execute(const std::string& commandLine);
void Draw(bool* open);

} // namespace Kuru::Console
