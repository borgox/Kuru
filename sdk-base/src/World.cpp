#include <Engine/World.hpp>

namespace Kuru::SDK {

UClass* USubsystem::StaticClass() { return UObject::FindClass("Core.Subsystem"); }
UClass* UScriptViewportClient::StaticClass() { return UObject::FindClass("Engine.ScriptViewportClient"); }
UClass* ULevelBase::StaticClass() { return UObject::FindClass("Engine.LevelBase"); }
UClass* ULevel::StaticClass() { return UObject::FindClass("Engine.Level"); }
UClass* UWorld::StaticClass() { return UObject::FindClass("Engine.World"); }
UClass* UEngine::StaticClass() { return UObject::FindClass("Engine.Engine"); }
UClass* UGameViewportClient::StaticClass() { return UObject::FindClass("Engine.GameViewportClient"); }
UClass* UGameEngine::StaticClass() { return UObject::FindClass("Engine.GameEngine"); }

} // namespace Kuru::SDK
