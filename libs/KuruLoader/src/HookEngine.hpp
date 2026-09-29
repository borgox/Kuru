#pragma once

#include <Kuru/HostApi.hpp>

#include <string>
#include <vector>



namespace Kuru::HookEngine {


bool Install(void* processEventAddress);           // slot 67 ProcessEvent
bool InstallProcessInternal(void* address);        // non-virtual (address from PE&CF intersection)
bool InstallProcessDelegate(void* address);        // slot 68 ProcessDelegate (RL fork, PE-shaped)
bool InstallCallDelegate(void* address);           // slot 69 CallDelegate (FName in arg1)
bool InstallProcessRemoteFunction(void* address);  // slot 71 ProcessRemoteFunction
bool InstallCallFunction(void* address);           // slot 76 CallFunction (UFunction in arg3)
void SetGNatives(void* table);                     // interpreter opcode->handler table (for CF arg reading)
void SetInterpreterGlobals(void* gProperty, void* gPropAddr, void* gPropObject, void* gRuntimeUCFlags);
void Shutdown();

// The API table pushed into each mod (via KuruInitHostApi).
[[nodiscard]] const Kuru::HostApi* Api();

// Remove every hook registered by a given mod (used when the mod is disabled).
void RemoveHooksForMod(void* mod);

} // namespace Kuru::HookEngine
