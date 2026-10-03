#pragma once

#include "Containers.hpp"
#include "Memory.hpp"
#include "Names.hpp"
#include "ObjectModel.hpp"
#include "Runtime.hpp"

#include <cstdint>

namespace Kuru::SDK {

extern TArray<FNameEntry*>* GNames;
extern TArray<UObject*>* GObjects;

extern FMalloc* GMalloc;
extern const char* BuildDate;
extern const wchar_t* GPsyonixBuildID;
extern std::uint32_t GPsyonixBuildNumber;

extern ProcessEventFunc ProcessEvent;
extern ProcessInternalFunc ProcessInternal;
extern ProcessDelegateFunc ProcessDelegate;

bool KuruInitGlobals();

} // namespace Kuru::SDK
