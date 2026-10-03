#pragma once

#include "Containers.hpp"
#include "Flags.hpp"
#include "Names.hpp"
#include "Strings.hpp"

#include <cstddef>
#include <cstdint>
#include <string>

namespace Kuru::SDK {

class UObject;
class UClass;
class UPackage;
class UField;
class UStruct;
class UScriptStruct;
class UState;
class UFunction;
class UProperty;
class UEnum;
class UConst;
class UMetaData;
class UTextBuffer;
class UObjectProperty;
class UClassProperty;
class UByteProperty;
class UBoolProperty;
class UIntProperty;
class UQWordProperty;
class USQWordProperty;
class UFloatProperty;
class UNameProperty;
class UStrProperty;
class UArrayProperty;
class UMapProperty;
class UStructProperty;
class UDelegateProperty;
class UInterfaceProperty;
class UComponentProperty;
struct FStateFrame;
struct FFrame;

using NativeFunctionPtr = void(__fastcall*)(UObject* object, FStateFrame& stack, void* result);

class UObject {
public:
    void * VfTableObject;             // 0x0000
    void * HashNext;                  // 0x0008
    std::uint64_t ObjectFlags;       // 0x0010
    void * HashOuterNext;             // 0x0018
    FStateFrame * StateFrame;         // 0x0020
    UObject * Linker;                 // 0x0028
    void * LinkerIndex;               // 0x0030, RL fork stores pointer-sized data here
    std::int32_t ObjectInternalInteger; // 0x0038
    std::int32_t NetIndex;           // 0x003C
    UObject * Outer;                  // 0x0040
    FName Name;                      // 0x0048
    UClass * Class;                   // 0x0050
    UObject * ObjectArchetype;        // 0x0058

    [[nodiscard]] static TArray<UObject*>* GetGlobalObjects();
    [[nodiscard]] static UClass* StaticClass();
    [[nodiscard]] static UClass* FindClass(const std::string& classFullName);
    [[nodiscard]] static UFunction* FindFunction(const std::string& functionFullName);
    [[nodiscard]] static UObject* FindObject(const std::string& objectFullName, UClass* requiredClass = nullptr);
    [[nodiscard]] static UObject* FindObject(const char* objectFullName, UClass* requiredClass = nullptr);
    [[nodiscard]] static UObject* FindObjectChecked(const std::string& objectFullName, UClass* requiredClass = nullptr);
    [[nodiscard]] static TArray<UObject*> FindObjects(const std::string& objectFullName, UClass* requiredClass = nullptr);

    template <typename T>
    [[nodiscard]] static T* FindObject(const std::string& objectFullName) {
        return reinterpret_cast<T*>(FindObject(objectFullName, T::StaticClass()));
    }

    template <typename T>
    [[nodiscard]] static T* FindObject(const char* objectFullName) {
        return reinterpret_cast<T*>(FindObject(objectFullName, T::StaticClass()));
    }

    [[nodiscard]] std::string GetName() const;
    void GetName(FString& result) const;
    void AppendName(FString& result) const;
    [[nodiscard]] std::string GetNameCPP() const;
    [[nodiscard]] std::string GetFullName() const;
    [[nodiscard]] std::string GetPathName() const;
    [[nodiscard]] std::string GetPackageName() const;
    [[nodiscard]] FName GetFName() const noexcept { return Name; }
    [[nodiscard]] UClass* GetClass() const noexcept { return Class; }
    [[nodiscard]] UObject* GetOuter() const noexcept { return Outer; }
    [[nodiscard]] UPackage* GetOutermost() const;
    [[nodiscard]] UObject* GetPackageObj() const;
    [[nodiscard]] std::int32_t GetIndex() const noexcept { return ObjectInternalInteger; }
    [[nodiscard]] FStateFrame* GetStateFrame() const noexcept { return StateFrame; }
    [[nodiscard]] UObject* GetArchetype() const noexcept { return ObjectArchetype; }
    [[nodiscard]] bool IsA(const UClass* objectClass) const;
    [[nodiscard]] bool IsA(std::int32_t objectInternalInteger) const;
    [[nodiscard]] bool HasAnyFlags(std::uint64_t flags) const noexcept { return (ObjectFlags & flags) != 0; }
    [[nodiscard]] bool HasAllFlags(std::uint64_t flags) const noexcept { return (ObjectFlags & flags) == flags; }
    [[nodiscard]] std::uint64_t GetMaskedFlags(std::uint64_t flags) const noexcept { return ObjectFlags & flags; }
    [[nodiscard]] bool IsPendingKill() const noexcept { return HasAnyFlags(RF_PendingKill); }
    [[nodiscard]] bool IsTemplate() const noexcept { return HasAnyFlags(RF_ClassDefaultObject | RF_ArchetypeObject); }
    [[nodiscard]] bool IsValidObject() const noexcept;
    [[nodiscard]] bool IsIn(const UObject* someOuter) const noexcept {
        for (const UObject* nextOuter = Outer; nextOuter != nullptr; nextOuter = nextOuter->Outer) {
            if (nextOuter == someOuter) {
                return true;
            }
        }
        return false;
    }

    template <typename T>
    [[nodiscard]] bool IsA() const {
        return IsA(T::StaticClass());
    }

    template <typename T>
    [[nodiscard]] T* GetTypedOuter() const {
        for (UObject* nextOuter = Outer; nextOuter != nullptr; nextOuter = nextOuter->Outer) {
            if (nextOuter->IsA(T::StaticClass())) {
                return reinterpret_cast<T*>(nextOuter);
            }
        }
        return nullptr;
    }

    [[nodiscard]] UObject* GetTypedOuter(UClass* targetClass) const;

    template <typename T>
    [[nodiscard]] T* GetArchetype() const {
        return ObjectArchetype && ObjectArchetype->IsA(T::StaticClass()) ? reinterpret_cast<T*>(ObjectArchetype) : nullptr;
    }

    void ProcessEvent(UFunction* function, void* params, void* result = nullptr);
    void ProcessEventChecked(UFunction* function, void* params, void* result = nullptr);
    void ProcessInternal(FFrame& stack, void* result = nullptr);
    void ProcessDelegate(FName delegateName, void* params, void* result = nullptr);
    void MarkPendingKill();
    void SetFlags(std::uint64_t flags) noexcept { ObjectFlags |= flags; }
    void ClearFlags(std::uint64_t flags) noexcept { ObjectFlags &= ~flags; }
    void ConditionalBeginDestroy();
};

template <typename T>
[[nodiscard]] T* FindObject(const std::string& objectFullName) {
    return UObject::FindObject<T>(objectFullName);
}

template <typename T>
[[nodiscard]] T* FindObject(const char* objectFullName) {
    return UObject::FindObject<T>(objectFullName);
}

template <typename T>
[[nodiscard]] T* FindObjectChecked(const std::string& objectFullName) {
    return reinterpret_cast<T*>(UObject::FindObjectChecked(objectFullName, T::StaticClass()));
}

template <typename T>
[[nodiscard]] UClass* StaticClass() {
    return T::StaticClass();
}

template <typename T>
[[nodiscard]] bool IsA(const UObject* object) {
    return object != nullptr && object->IsA(T::StaticClass());
}

template <typename T>
[[nodiscard]] T* Cast(UObject* object) {
    return object && object->IsA(T::StaticClass()) ? reinterpret_cast<T*>(object) : nullptr;
}

template <typename T>
[[nodiscard]] T* ExactCast(UObject* object) {
    return object && object->GetClass() == T::StaticClass() ? reinterpret_cast<T*>(object) : nullptr;
}

template <typename T>
[[nodiscard]] T* As(UObject* object) {
    return Cast<T>(object);
}

class UField : public UObject {
public:
    UField* Next;                    // 0x0060
    std::uint8_t Pad_0068[0x08];     // 0x0068 alignment padding to UField size

    [[nodiscard]] static UClass* StaticClass();
    void Bind();
    [[nodiscard]] UClass* GetOwnerClass() const;
    [[nodiscard]] UStruct* GetOwnerStruct() const;
    void AddCppProperty(UProperty* property);
    void MergeBools();
};

class UStruct : public UField {
public:
    UTextBuffer* ScriptText;          // 0x0070, editor/source text buffer when present
    UTextBuffer* CppText;             // 0x0078
    UStruct* SuperStruct;            // 0x0080
    UField* Children;                // 0x0088
    std::int32_t PropertiesSize;     // 0x0090
    std::uint32_t Pad_0094;          // 0x0094 alignment before Script
    TArray<std::uint8_t> Script;     // 0x0098 raw bytecode bytes
    std::int32_t TextPos;            // 0x00A8, stock compiler source position
    std::int32_t Line;               // 0x00AC, stock compiler source line
    std::int32_t MinAlignment;       // 0x00B0
    std::uint32_t Pad_00B4;          // 0x00B4
    std::uint8_t RLPad_00B8[0x50];   // 0x00B8, RL-private struct tail before linked property chains
    UProperty* RefLink;              // 0x0108, first property containing object references
    UProperty* PropertyLink;         // 0x0110, first linked property; runtime-verified on Core.Object.Rotator
    UProperty* ConstructorLink;      // 0x0118, first property requiring construction/destruction
    TArray<UObject*> ScriptObjectReferences; // 0x0120, object references embedded in script bytecode

    [[nodiscard]] static UClass* StaticClass();
    [[nodiscard]] UStruct* GetSuperStruct() const noexcept { return SuperStruct; }
    [[nodiscard]] UStruct* GetInheritanceSuper() const noexcept { return SuperStruct; }
    [[nodiscard]] UField* GetChildren() const noexcept { return Children; }
    [[nodiscard]] UProperty* GetRefLink() const noexcept { return RefLink; }
    [[nodiscard]] UProperty* GetPropertyLink() const noexcept { return PropertyLink; }
    [[nodiscard]] UProperty* GetConstructorLink() const noexcept { return ConstructorLink; }
    [[nodiscard]] std::int32_t GetPropertiesSize() const noexcept { return PropertiesSize; }
    [[nodiscard]] std::int32_t GetMinAlignment() const noexcept { return MinAlignment; }
    [[nodiscard]] bool IsChildOf(const UStruct* someBase) const noexcept {
        for (const UStruct* current = this; current != nullptr; current = current->SuperStruct) {
            if (current == someBase) {
                return true;
            }
        }
        return false;
    }
    [[nodiscard]] const char* GetPrefixCPP() const { return "F"; }
    void Link(void* archive, bool relinkExistingProperties);
    void SerializeBin(void* archive, std::uint8_t* data, std::int32_t maxReadBytes) const;
    void SerializeTaggedProperties(void* archive, std::uint8_t* data, UStruct* defaultsStruct, std::uint8_t* defaults,
        std::int32_t defaultsCount = 0) const;
    [[nodiscard]] UProperty* FindPropertyByName(FName propertyName) const;
    [[nodiscard]] UFunction* FindFunctionByName(FName functionName) const;
};

class UScriptStruct : public UStruct {
public:
    FString DefaultStructPropText;    // 0x0130
    std::uint32_t StructFlags;        // 0x0140
    std::uint32_t Pad_0144;           // 0x0144
    TArray<std::uint8_t> StructDefaults; // 0x0148

    [[nodiscard]] static UClass* StaticClass();
    [[nodiscard]] std::uint8_t* GetDefaults() noexcept { return StructDefaults.Data; }
    [[nodiscard]] const std::uint8_t* GetDefaults() const noexcept { return StructDefaults.Data; }
    [[nodiscard]] std::int32_t GetDefaultsCount() const noexcept { return StructDefaults.Count; }
    [[nodiscard]] TArray<std::uint8_t>& GetDefaultArray() noexcept { return StructDefaults; }
    [[nodiscard]] const TArray<std::uint8_t>& GetDefaultArray() const noexcept { return StructDefaults; }
    [[nodiscard]] bool HasAnyStructFlags(std::uint32_t flags) const noexcept { return (StructFlags & flags) != 0; }
    [[nodiscard]] bool ShouldSerializeAtomically(bool archiveContainsCookedData = false) const noexcept {
        return (StructFlags & STRUCT_Atomic) != 0 ||
            (archiveContainsCookedData && (StructFlags & STRUCT_AtomicWhenCooked) != 0);
    }
    void AllocateStructDefaults();
    void PropagateStructDefaults();
};

class UFunction : public UStruct {
public:
    std::uint64_t FunctionFlags;     // 0x0130
    std::uint16_t iNative;           // 0x0138
    std::uint16_t RepOffset;         // 0x013A
    FName FriendlyName;              // 0x013C
    std::uint8_t OperPrecedence;     // 0x0144
    std::uint8_t NumParms;           // 0x0145
    std::uint16_t ParmsSize;         // 0x0146
    std::uint16_t ReturnValueOffset; // 0x0148
    std::uint8_t Pad_014A[0x06];     // 0x014A alignment before FirstStructWithDefaults
    UStructProperty* FirstStructWithDefaults; // 0x0150
    NativeFunctionPtr Func;           // 0x0158

    [[nodiscard]] static UClass* StaticClass();
    [[nodiscard]] static UFunction* FindFunction(const std::string& functionFullName);
    [[nodiscard]] bool HasAnyFunctionFlags(std::uint32_t flags) const noexcept { return (FunctionFlags & flags) != 0; }
    [[nodiscard]] bool HasAllFunctionFlags(std::uint32_t flags) const noexcept { return (FunctionFlags & flags) == flags; }
    [[nodiscard]] UProperty* GetReturnProperty() const;
};

class UState : public UStruct {
public:
    std::uint32_t ProbeMask;         // 0x0130
    std::uint32_t StateFlags;        // 0x0134
    std::uint16_t LabelTableOffset;  // 0x0138
    std::uint8_t Pad_013A[0x06];     // 0x013A alignment before FuncMap
    TMap<FName, UFunction*> FuncMap; // 0x0140

    [[nodiscard]] static UClass* StaticClass();
    [[nodiscard]] UFunction* FindFunctionByName(FName functionName) const;
    [[nodiscard]] bool HasAnyStateFlags(std::uint32_t flags) const noexcept { return (StateFlags & flags) != 0; }
    [[nodiscard]] UState* GetSuperState() const noexcept { return reinterpret_cast<UState*>(SuperStruct); }
};

struct FRepRecord {
    UProperty* Property;             // 0x0000
    std::int32_t Index;              // 0x0008
    std::uint32_t Pad_000C;          // 0x000C
};

struct FImplementedInterface {
    UClass* Class = nullptr;         // 0x0000
    UProperty* PointerProperty = nullptr; // 0x0008
};

struct FGCReferenceTokenStream {
    TArray<std::uint32_t> Tokens;    // 0x0000
};

struct FNativeInlineBitArray4Mirror {
    std::uint32_t InlineData[4]{};   // 0x0000
    void* SecondaryData = nullptr;   // 0x0010
    std::int32_t NumBits = 0;        // 0x0018
    std::int32_t MaxBits = 0;        // 0x001C
};

struct FNativeSparseArrayMirror {
    void* Data = nullptr;            // 0x0000
    std::int32_t Count = 0;          // 0x0008
    std::int32_t Max = 0;            // 0x000C
    FNativeInlineBitArray4Mirror AllocationFlags; // 0x0010
    std::int32_t FirstFreeIndex = 0; // 0x0030
    std::int32_t NumFreeIndices = 0; // 0x0034
};

struct FNativeInlineHashAllocator1Mirror {
    std::int32_t InlineHash = 0;     // 0x0000, FSetElementId
    std::uint32_t Pad_0004 = 0;      // 0x0004
    void* SecondaryData = nullptr;   // 0x0008
};

struct FNativeSetMirror {
    FNativeSparseArrayMirror Elements; // 0x0000
    FNativeInlineHashAllocator1Mirror Hash; // 0x0038
    std::int32_t HashSize = 0;       // 0x0048
    std::uint32_t Pad_004C = 0;      // 0x004C

    [[nodiscard]] std::int32_t Num() const noexcept { return Elements.Count - Elements.NumFreeIndices; }
    [[nodiscard]] bool IsEmpty() const noexcept { return Num() <= 0; }
    [[nodiscard]] bool UsesInlineHash() const noexcept { return Hash.SecondaryData == nullptr; }
};

struct FRLClassNativeCache : FNativeSetMirror {
    // RL runtime class cache before ClassWithin. Element type still needs semantic RE.
};

struct FRLClassTailCache : FNativeSetMirror {
    // RL runtime class cache after ReferenceTokenStream. Element type still needs semantic RE.
};

class UClass : public UState {
public:
    std::uint32_t ClassFlags;        // 0x0190
    std::uint32_t ClassCastFlags;    // 0x0194, stock/RL reflected EClassCastFlags for normal classes
    FRLClassNativeCache RLNativeCache; // 0x0198, RL class cache before cast flags
    std::uint64_t RLClassCastFlags;  // 0x01E8, verified metaclass cast chain
    UClass* ClassWithin;             // 0x01F0
    FName ClassConfigName;           // 0x01F8
    TArray<FRepRecord> ClassReps;    // 0x0200
    TArray<UField*> NetFields;       // 0x0210
    TArray<FName> HideCategories;    // 0x0220
    TArray<FName> AutoExpandCategories; // 0x0230
    TArray<FName> AutoCollapseCategories; // 0x0240
    TArray<FName> DontSortCategories; // 0x0250
    TArray<FName> DependentOn;       // 0x0260
    TArray<FName> ClassGroupNames;   // 0x0270
    std::uint32_t bForceScriptOrder; // 0x0280
    std::uint32_t Pad_0284;          // 0x0284
    FString ClassHeaderFilename;     // 0x0288
    FName DLLBindName;               // 0x0298, stock WITH_LIBFFI native binding name
    void* DLLBindHandle;             // 0x02A0, stock WITH_LIBFFI binding handle
    std::uint8_t RLPad_02A8[0x18];   // 0x02A8, RL tail between stock libffi fields and CDO
    UObject* ClassDefaultObject;     // 0x02C0
    void* ClassConstructor;          // 0x02C8
    void* ClassStaticConstructor;    // 0x02D0
    void* ClassStaticInitializer;    // 0x02D8
    TMap<FName, UObject*> ComponentNameToDefaultObjectMap; // 0x02E0
    TArray<FImplementedInterface> Interfaces; // 0x0330
    FString DefaultPropText;         // 0x0340
    std::uint32_t bNeedsPropertiesLinked; // 0x0350
    std::uint32_t Pad_0354;          // 0x0354
    FGCReferenceTokenStream ReferenceTokenStream; // 0x0358
    FRLClassTailCache RLTailCache;   // 0x0368, RL class tail cache

    [[nodiscard]] static UClass* StaticClass();
    [[nodiscard]] bool HasAnyClassFlags(std::uint32_t flags) const noexcept { return (ClassFlags & flags) != 0; }
    [[nodiscard]] bool HasAllClassFlags(std::uint32_t flags) const noexcept { return (ClassFlags & flags) == flags; }
    [[nodiscard]] std::uint64_t GetEffectiveCastFlags() const noexcept {
        return static_cast<std::uint64_t>(ClassCastFlags) | RLClassCastFlags;
    }
    [[nodiscard]] bool HasAnyCastFlag(std::uint64_t flags) const noexcept { return (GetEffectiveCastFlags() & flags) != 0; }
    [[nodiscard]] bool HasAllCastFlags(std::uint64_t flags) const noexcept {
        return (GetEffectiveCastFlags() & flags) == flags;
    }
    [[nodiscard]] bool IsChildOf(const UClass* someBase) const noexcept {
        return UState::IsChildOf(reinterpret_cast<const UStruct*>(someBase));
    }
    [[nodiscard]] bool ImplementsInterface(const UClass* someInterface) const;
    [[nodiscard]] UObject* GetDefaultObject() const noexcept { return ClassDefaultObject; }
    [[nodiscard]] UObject* FindDefaultObject() const noexcept { return ClassDefaultObject; }
    [[nodiscard]] UProperty* FindPropertyByName(FName propertyName) const;
    [[nodiscard]] UFunction* FindFunctionByName(FName functionName) const;
};

class UProperty : public UField {
public:
    std::int32_t ArrayDim;           // 0x0070
    std::int32_t ElementSize;        // 0x0074
    std::uint64_t PropertyFlags;     // 0x0078
    std::uint16_t RepOffset;         // 0x0080
    std::uint16_t RepIndex;          // 0x0082
    std::uint32_t Pad_0084;          // 0x0084
    FName Category;                  // 0x0088
    UEnum* ArraySizeEnum;            // 0x0090
    std::int32_t Offset;             // 0x0098
    std::uint32_t Pad_009C;          // 0x009C
    UProperty* PropertyLinkNext;     // 0x00A0
    UProperty* ConstructorLinkNext;  // 0x00A8
    UProperty* NextRef;              // 0x00B0
    FString RLPropertyName;          // 0x00B8, RL metadata string mirroring the property's runtime name

    [[nodiscard]] static UClass* StaticClass();
    [[nodiscard]] std::int32_t GetSize() const noexcept { return ArrayDim * ElementSize; }
    [[nodiscard]] std::int32_t GetOffset_ForGC() const noexcept { return Offset; }
    [[nodiscard]] std::int32_t GetOffset_ForUFunction() const noexcept { return Offset; }
    [[nodiscard]] bool HasAnyPropertyFlags(std::uint64_t flags) const noexcept { return (PropertyFlags & flags) != 0; }
    [[nodiscard]] bool HasAllPropertyFlags(std::uint64_t flags) const noexcept { return (PropertyFlags & flags) == flags; }
    [[nodiscard]] bool Port(std::uint64_t flags = 0) const noexcept { return (PropertyFlags & flags) == flags; }
    [[nodiscard]] std::string GetNameCPP() const;
    [[nodiscard]] FString GetCPPType(FString* extendedTypeText = nullptr, std::uint32_t cppExportFlags = 0) const;
    [[nodiscard]] bool ContainsObjectReference() const;
    [[nodiscard]] bool ContainsInstancedObjectProperty() const;
    [[nodiscard]] bool RequiresInit() const;
    [[nodiscard]] std::int32_t GetMinAlignment() const;
    template <typename T>
    [[nodiscard]] T* ContainerPtrToValuePtr(void* container) const {
        return reinterpret_cast<T*>(static_cast<std::uint8_t*>(container) + Offset);
    }
    template <typename T>
    [[nodiscard]] const T* ContainerPtrToValuePtr(const void* container) const {
        return reinterpret_cast<const T*>(static_cast<const std::uint8_t*>(container) + Offset);
    }
    [[nodiscard]] FString GetValueAsString(const void* container) const;
};

template <typename T>
[[nodiscard]] T* GetDefaultObject() {
    UClass* objectClass = T::StaticClass();
    return objectClass ? reinterpret_cast<T*>(objectClass->GetDefaultObject()) : nullptr;
}

template <typename T>
[[nodiscard]] T* GetMutableDefaultObject() {
    return GetDefaultObject<T>();
}

class UObjectProperty : public UProperty {
public:
    UClass* PropertyClass;           // 0x00C8
    std::uint8_t RLPad_00D0[0x08];   // 0x00D0, RL reserves tail before ClassProperty::MetaClass

    [[nodiscard]] static UClass* StaticClass();
};

class UClassProperty : public UObjectProperty {
public:
    UClass* MetaClass;               // 0x00D8

    [[nodiscard]] static UClass* StaticClass();
};

class UByteProperty : public UProperty {
public:
    UEnum* Enum;                     // 0x00C8

    [[nodiscard]] static UClass* StaticClass();
};

class UBoolProperty : public UProperty {
public:
    std::uint64_t BitMask;           // 0x00C8, verify exact RL bool mask semantics

    [[nodiscard]] static UClass* StaticClass();
};

class UIntProperty : public UProperty { public: [[nodiscard]] static UClass* StaticClass(); };
class UQWordProperty : public UProperty { public: [[nodiscard]] static UClass* StaticClass(); };
class USQWordProperty : public UProperty { public: [[nodiscard]] static UClass* StaticClass(); };
class UFloatProperty : public UProperty { public: [[nodiscard]] static UClass* StaticClass(); };
class UNameProperty : public UProperty { public: [[nodiscard]] static UClass* StaticClass(); };
class UStrProperty : public UProperty { public: [[nodiscard]] static UClass* StaticClass(); };

class UArrayProperty : public UProperty {
public:
    UProperty* Inner;                // 0x00C8

    [[nodiscard]] static UClass* StaticClass();
};

class UMapProperty : public UProperty {
public:
    UProperty* Key;                  // 0x00C8
    UProperty* Value;                // 0x00D0

    [[nodiscard]] static UClass* StaticClass();
};

class UStructProperty : public UProperty {
public:
    UScriptStruct* Struct;           // 0x00C8

    [[nodiscard]] static UClass* StaticClass();
};

class UDelegateProperty : public UProperty {
public:
    UFunction* Function;             // 0x00C8

    [[nodiscard]] static UClass* StaticClass();
};

class UInterfaceProperty : public UProperty {
public:
    UClass* InterfaceClass;          // 0x00C8
    std::uint8_t RLPad_00D0[0x08];   // 0x00D0, RL property_size tail

    [[nodiscard]] static UClass* StaticClass();
};

class UComponentProperty : public UObjectProperty { public: [[nodiscard]] static UClass* StaticClass(); };

class UEnum : public UField {
public:
    TArray<FName> Names;             // 0x0070

    [[nodiscard]] static UClass* StaticClass();
    [[nodiscard]] FName GetEnumName(std::int32_t index) const;
    [[nodiscard]] std::int32_t FindEnumIndex(FName enumName) const;
    [[nodiscard]] std::string GetNameStringByIndex(std::int32_t index) const;
    [[nodiscard]] std::int32_t GetValueByName(FName enumName) const;
};

class UConst : public UField {
public:
    FString Value;                   // 0x0070

    [[nodiscard]] static UClass* StaticClass();
};

class UMetaData : public UObject {
public:
    TMap<UObject*, TMap<FName, FString>> ObjectMetaDataMap; // 0x0060

    [[nodiscard]] static UClass* StaticClass();
    [[nodiscard]] const FString& GetValue(const UObject* object, const char* key) const;
    [[nodiscard]] const FString& GetValue(const UObject* object, FName key) const;
    [[nodiscard]] bool HasValue(const UObject* object, const char* key) const;
    [[nodiscard]] bool HasValue(const UObject* object, FName key) const;
    [[nodiscard]] bool HasObjectValues(const UObject* object) const;
    void SetObjectValues(const UObject* object, const TMap<FName, FString>& values);
    void SetValue(const UObject* object, const char* key, const FString& value);
    void SetValue(const UObject* object, FName key, const FString& value);
    static bool AttemptParseIntrinsicMetaData(const UClass& classToUse, const UObject& objectToUse, UMetaData& metaDataToAddTo);
};

#if defined(__GNUC__)
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Winvalid-offsetof"
#endif

static_assert(sizeof(UObject) == 0x60);
static_assert(offsetof(UObject, Outer) == 0x40);
static_assert(offsetof(UObject, StateFrame) == 0x20);
static_assert(offsetof(UObject, Name) == 0x48);
static_assert(offsetof(UObject, Class) == 0x50);
static_assert(sizeof(UField) == 0x70);
static_assert(sizeof(UStruct) == 0x130);
static_assert(offsetof(UStruct, SuperStruct) == 0x80);
static_assert(offsetof(UStruct, Children) == 0x88);
static_assert(offsetof(UStruct, PropertiesSize) == 0x90);
static_assert(offsetof(UStruct, Script) == 0x98);
static_assert(offsetof(UStruct, TextPos) == 0xA8);
static_assert(offsetof(UStruct, Line) == 0xAC);
static_assert(offsetof(UStruct, MinAlignment) == 0xB0);
static_assert(offsetof(UStruct, RefLink) == 0x108);
static_assert(offsetof(UStruct, PropertyLink) == 0x110);
static_assert(offsetof(UStruct, ConstructorLink) == 0x118);
static_assert(offsetof(UStruct, ScriptObjectReferences) == 0x120);
static_assert(sizeof(UScriptStruct) == 0x158);
static_assert(offsetof(UScriptStruct, DefaultStructPropText) == 0x130);
static_assert(offsetof(UScriptStruct, StructFlags) == 0x140);
static_assert(offsetof(UScriptStruct, StructDefaults) == 0x148);
static_assert(sizeof(UFunction) == 0x160);
static_assert(offsetof(UFunction, FunctionFlags) == 0x130);
static_assert(offsetof(UFunction, FirstStructWithDefaults) == 0x150);
static_assert(offsetof(UFunction, Func) == 0x158);
static_assert(sizeof(UState) == 0x190);
static_assert(sizeof(FRepRecord) == 0x10);
static_assert(sizeof(FImplementedInterface) == 0x10);
static_assert(sizeof(FGCReferenceTokenStream) == 0x10);
static_assert(sizeof(FNativeInlineBitArray4Mirror) == 0x20);
static_assert(offsetof(FNativeInlineBitArray4Mirror, SecondaryData) == 0x10);
static_assert(offsetof(FNativeInlineBitArray4Mirror, NumBits) == 0x18);
static_assert(sizeof(FNativeSparseArrayMirror) == 0x38);
static_assert(offsetof(FNativeSparseArrayMirror, AllocationFlags) == 0x10);
static_assert(offsetof(FNativeSparseArrayMirror, FirstFreeIndex) == 0x30);
static_assert(sizeof(FNativeInlineHashAllocator1Mirror) == 0x10);
static_assert(offsetof(FNativeInlineHashAllocator1Mirror, SecondaryData) == 0x08);
static_assert(sizeof(FNativeSetMirror) == 0x50);
static_assert(offsetof(FNativeSetMirror, Hash) == 0x38);
static_assert(offsetof(FNativeSetMirror, HashSize) == 0x48);
static_assert(sizeof(FRLClassNativeCache) == 0x50);
static_assert(sizeof(FRLClassTailCache) == 0x50);
static_assert(offsetof(UState, ProbeMask) == 0x130);
static_assert(offsetof(UState, FuncMap) == 0x140);
static_assert(sizeof(UClass) == 0x3B8);
static_assert(offsetof(UClass, ClassCastFlags) == 0x194);
static_assert(offsetof(UClass, RLNativeCache) == 0x198);
static_assert(offsetof(UClass, RLClassCastFlags) == 0x1E8);
static_assert(offsetof(UClass, ClassWithin) == 0x1F0);
static_assert(offsetof(UClass, ClassReps) == 0x200);
static_assert(offsetof(UClass, NetFields) == 0x210);
static_assert(offsetof(UClass, ClassHeaderFilename) == 0x288);
static_assert(offsetof(UClass, DLLBindName) == 0x298);
static_assert(offsetof(UClass, DLLBindHandle) == 0x2A0);
static_assert(offsetof(UClass, RLPad_02A8) == 0x2A8);
static_assert(offsetof(UClass, ClassDefaultObject) == 0x2C0);
static_assert(offsetof(UClass, ComponentNameToDefaultObjectMap) == 0x2E0);
static_assert(offsetof(UClass, Interfaces) == 0x330);
static_assert(offsetof(UClass, DefaultPropText) == 0x340);
static_assert(offsetof(UClass, ReferenceTokenStream) == 0x358);
static_assert(offsetof(UClass, RLTailCache) == 0x368);
static_assert(sizeof(UProperty) == 0xC8);
static_assert(offsetof(UProperty, Offset) == 0x98);
static_assert(offsetof(UProperty, PropertyLinkNext) == 0xA0);
static_assert(offsetof(UProperty, ConstructorLinkNext) == 0xA8);
static_assert(offsetof(UProperty, NextRef) == 0xB0);
static_assert(offsetof(UProperty, RLPropertyName) == 0xB8);
static_assert(sizeof(UObjectProperty) == 0xD8);
static_assert(offsetof(UObjectProperty, PropertyClass) == 0xC8);
static_assert(offsetof(UObjectProperty, RLPad_00D0) == 0xD0);
static_assert(sizeof(UClassProperty) == 0xE0);
static_assert(offsetof(UClassProperty, MetaClass) == 0xD8);
static_assert(sizeof(UByteProperty) == 0xD0);
static_assert(offsetof(UByteProperty, Enum) == 0xC8);
static_assert(sizeof(UBoolProperty) == 0xD0);
static_assert(offsetof(UBoolProperty, BitMask) == 0xC8);
static_assert(sizeof(UIntProperty) == 0xC8);
static_assert(sizeof(UQWordProperty) == 0xC8);
static_assert(sizeof(USQWordProperty) == 0xC8);
static_assert(sizeof(UFloatProperty) == 0xC8);
static_assert(sizeof(UNameProperty) == 0xC8);
static_assert(sizeof(UStrProperty) == 0xC8);
static_assert(sizeof(UArrayProperty) == 0xD0);
static_assert(offsetof(UArrayProperty, Inner) == 0xC8);
static_assert(sizeof(UMapProperty) == 0xD8);
static_assert(offsetof(UMapProperty, Key) == 0xC8);
static_assert(offsetof(UMapProperty, Value) == 0xD0);
static_assert(sizeof(UStructProperty) == 0xD0);
static_assert(offsetof(UStructProperty, Struct) == 0xC8);
static_assert(sizeof(UDelegateProperty) == 0xD0);
static_assert(offsetof(UDelegateProperty, Function) == 0xC8);
static_assert(sizeof(UInterfaceProperty) == 0xD8);
static_assert(offsetof(UInterfaceProperty, InterfaceClass) == 0xC8);
static_assert(offsetof(UInterfaceProperty, RLPad_00D0) == 0xD0);
static_assert(sizeof(UComponentProperty) == 0xD8);
static_assert(sizeof(UMetaData) == 0xB0);
static_assert(offsetof(UMetaData, ObjectMetaDataMap) == 0x60);

#if defined(__GNUC__)
#pragma GCC diagnostic pop
#endif

} // namespace Kuru::SDK
