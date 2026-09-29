#pragma once

#include <cstddef>
#include <string_view>

namespace Kuru::SDK::Metadata {

struct ReflectedTypeBinding {
    std::string_view FullName;
    std::string_view CppName;
};

inline constexpr ReflectedTypeBinding BaseReflectedTypes[] = {
    {"Core.Object", "Kuru::SDK::UObject"},
    {"Core.Field", "Kuru::SDK::UField"},
    {"Core.Struct", "Kuru::SDK::UStruct"},
    {"Core.ScriptStruct", "Kuru::SDK::UScriptStruct"},
    {"Core.State", "Kuru::SDK::UState"},
    {"Core.Class", "Kuru::SDK::UClass"},
    {"Core.Function", "Kuru::SDK::UFunction"},
    {"Core.Property", "Kuru::SDK::UProperty"},
    {"Core.ObjectProperty", "Kuru::SDK::UObjectProperty"},
    {"Core.ClassProperty", "Kuru::SDK::UClassProperty"},
    {"Core.ByteProperty", "Kuru::SDK::UByteProperty"},
    {"Core.BoolProperty", "Kuru::SDK::UBoolProperty"},
    {"Core.IntProperty", "Kuru::SDK::UIntProperty"},
    {"Core.QWordProperty", "Kuru::SDK::UQWordProperty"},
    {"Core.SQWordProperty", "Kuru::SDK::USQWordProperty"},
    {"Core.FloatProperty", "Kuru::SDK::UFloatProperty"},
    {"Core.NameProperty", "Kuru::SDK::UNameProperty"},
    {"Core.StrProperty", "Kuru::SDK::UStrProperty"},
    {"Core.ArrayProperty", "Kuru::SDK::UArrayProperty"},
    {"Core.MapProperty", "Kuru::SDK::UMapProperty"},
    {"Core.StructProperty", "Kuru::SDK::UStructProperty"},
    {"Core.DelegateProperty", "Kuru::SDK::UDelegateProperty"},
    {"Core.InterfaceProperty", "Kuru::SDK::UInterfaceProperty"},
    {"Core.ComponentProperty", "Kuru::SDK::UComponentProperty"},
    {"Core.Enum", "Kuru::SDK::UEnum"},
    {"Core.Const", "Kuru::SDK::UConst"},
    {"Core.MetaData", "Kuru::SDK::UMetaData"},
    {"Core.Object.Vector", "Kuru::SDK::FVector"},
    {"Core.Object.Vector2D", "Kuru::SDK::FVector2D"},
    {"Core.Object.Vector4", "Kuru::SDK::FVector4"},
    {"Core.Object.Plane", "Kuru::SDK::FPlane"},
    {"Core.Object.Rotator", "Kuru::SDK::FRotator"},
    {"Core.Object.Color", "Kuru::SDK::FColor"},
    {"Core.Object.LinearColor", "Kuru::SDK::FLinearColor"},
    {"Core.Object.Guid", "Kuru::SDK::FGuid"},
    {"Core.Object.Pointer", "Kuru::SDK::FPointer"},
    {"Core.Linker", "Kuru::SDK::ULinker"},
    {"Core.Package", "Kuru::SDK::UPackage"},
    {"Core.PackageMap", "Kuru::SDK::UPackageMap"},
    {"Core.LinkerLoad", "Kuru::SDK::ULinkerLoad"},
    {"Core.LinkerSave", "Kuru::SDK::ULinkerSave"},
    {"Core.Subsystem", "Kuru::SDK::USubsystem"},
    {"Engine.World", "Kuru::SDK::UWorld"},
    {"Engine.LevelBase", "Kuru::SDK::ULevelBase"},
    {"Engine.Level", "Kuru::SDK::ULevel"},
    {"Engine.ScriptViewportClient", "Kuru::SDK::UScriptViewportClient"},
    {"Engine.Engine", "Kuru::SDK::UEngine"},
    {"Engine.GameEngine", "Kuru::SDK::UGameEngine"},
    {"Engine.GameViewportClient", "Kuru::SDK::UGameViewportClient"},
    {"Engine.Surface", "Kuru::SDK::USurface"},
    {"Engine.Texture", "Kuru::SDK::UTexture"},
    {"Engine.Texture2D", "Kuru::SDK::UTexture2D"},
};

[[nodiscard]] constexpr bool IsBaseReflectedType(std::string_view fullName) {
    for (const ReflectedTypeBinding& binding : BaseReflectedTypes) {
        if (binding.FullName == fullName) {
            return true;
        }
    }
    return false;
}

[[nodiscard]] constexpr std::string_view FindBaseReflectedCppName(std::string_view fullName) {
    for (const ReflectedTypeBinding& binding : BaseReflectedTypes) {
        if (binding.FullName == fullName) {
            return binding.CppName;
        }
    }
    return {};
}

[[nodiscard]] constexpr std::size_t BaseReflectedTypeCount() {
    return std::size(BaseReflectedTypes);
}

} // namespace Kuru::SDK::Metadata
