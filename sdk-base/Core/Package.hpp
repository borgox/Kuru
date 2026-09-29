#pragma once

#include "Containers.hpp"
#include "Misc.hpp"
#include "Names.hpp"
#include "ObjectModel.hpp"
#include "PropertyTag.hpp"

#include <cstdint>
#include <cstddef>

namespace Kuru::SDK {

struct FGenerationInfo {
    std::int32_t ExportCount = 0;
    std::int32_t NameCount = 0;
    std::int32_t NetObjectCount = 0;
};

struct FLevelGuids {
    FName LevelName;
    TArray<FGuid> Guids;
};

struct FPackageFileSummary {
    std::uint32_t Tag = 0;
    std::int32_t FileVersion = 0;
    std::int32_t LicenseeVersion = 0;
    std::uint32_t PackageFlags = 0;
    std::int32_t NameCount = 0;
    std::int32_t NameOffset = 0;
    std::int32_t ExportCount = 0;
    std::int32_t ExportOffset = 0;
    std::int32_t ImportCount = 0;
    std::int32_t ImportOffset = 0;
    FGuid Guid;
    TArray<FGenerationInfo> Generations;
    std::int32_t DependsOffset = 0;
    std::int32_t ImportExportGuidsOffset = 0;
    std::int32_t ImportGuidsCount = 0;
    std::int32_t ExportGuidsCount = 0;
    std::int32_t ThumbnailTableOffset = 0;
    std::int32_t PackageSource = 0;
    TArray<FCompressedChunk> CompressedChunks;
    std::uint32_t CompressionFlags = 0;
    TArray<std::int32_t> AdditionalPackagesToCook;
    std::int32_t TextureAllocationsOffset = 0;
};

struct FObjectResource {
    FName ObjectName;
    std::int32_t OuterIndex = 0;
};

struct FObjectImport {
    FName ObjectName;
    std::int32_t OuterIndex = 0;
    FName ClassPackage;
    FName ClassName;

    [[nodiscard]] bool IsNull() const noexcept { return OuterIndex == 0 && ObjectName.IsNone(); }
};

struct FObjectExport {
    FName ObjectName;
    std::int32_t OuterIndex = 0;
    std::int32_t ClassIndex = 0;
    std::int32_t SuperIndex = 0;
    std::int32_t ArchetypeIndex = 0;
    std::uint64_t ObjectFlags = 0;
    std::int32_t SerialSize = 0;
    std::int32_t SerialOffset = 0;
    std::int32_t ScriptSerializationStartOffset = 0;
    std::int32_t ScriptSerializationEndOffset = 0;
    UObject* Object = nullptr;
    std::int32_t HashNext = -1;
    std::int32_t ExportFlags = 0;
    TArray<std::int32_t> GenerationNetObjectCount;
    FGuid PackageGuid;
    std::uint32_t PackageFlags = 0;

    [[nodiscard]] bool IsForcedExport() const noexcept { return (ExportFlags & 0x1) != 0; }
};

struct FStructEventMapMirror {
    void* PairsData = nullptr;        // 0x0000, native TMap pair storage
    std::int32_t PairsCount = 0;      // 0x0008
    std::int32_t PairsMax = 0;        // 0x000C
    void* AllocationFlagsData = nullptr; // 0x0010
    std::int32_t AllocationFlagsCount = 0; // 0x0018
    std::int32_t AllocationFlagsMax = 0; // 0x001C
    std::int32_t FirstFreeIndex = 0;  // 0x0020
    std::int32_t NumFreeIndices = 0;  // 0x0024
    std::int32_t* Hash = nullptr;     // 0x0028
    std::int32_t HashCount = 0;       // 0x0030
    std::uint32_t Pad_0034 = 0;       // 0x0034
    std::uint64_t InlineHash = 0;     // 0x0038
};

struct FUntypedBulkData;
struct FPatchReader;
struct FScriptPatchData;
struct FPatchData;
struct FEnumPatchData;

struct FArchiveStateMirror {
    FPointer VfTable_FArchive;       // 0x0000, secondary native FArchive vtable
    std::int32_t ArVer = 0;          // 0x0008
    std::int32_t ArNetVer = 0;       // 0x000C
    std::int32_t ArLicenseeVer = 0;  // 0x0010
    std::uint32_t ArIsLoading = 0;   // 0x0014, UBOOL
    std::uint32_t ArIsSaving = 0;    // 0x0018, UBOOL
    std::uint32_t ArIsTransacting = 0; // 0x001C, UBOOL
    std::uint32_t ArWantBinaryPropertySerialization = 0; // 0x0020, UBOOL
    std::uint32_t ArForceUnicode = 0; // 0x0024, UBOOL
    std::uint32_t ArIsPersistent = 0; // 0x0028, UBOOL
    std::uint32_t ArForEdit = 0;     // 0x002C, UBOOL
    std::uint32_t ArForClient = 0;   // 0x0030, UBOOL
    std::uint32_t ArForServer = 0;   // 0x0034, UBOOL
    std::uint32_t ArIsError = 0;     // 0x0038, UBOOL
    std::uint32_t ArIsCriticalError = 0; // 0x003C, UBOOL
    std::uint32_t ArContainsCookedData = 0; // 0x0040, UBOOL
    std::uint32_t ArContainsCode = 0; // 0x0044, UBOOL
    std::uint32_t ArContainsMap = 0; // 0x0048, UBOOL
    std::uint32_t ArForceByteSwapping = 0; // 0x004C, UBOOL
    std::int32_t ArSerializingDefaults = 0; // 0x0050
    std::uint32_t ArIgnoreArchetypeRef = 0; // 0x0054, UBOOL
    std::uint32_t ArIgnoreOuterRef = 0; // 0x0058, UBOOL
    std::uint32_t ArIgnoreClassRef = 0; // 0x005C, UBOOL
    std::int32_t ArAllowEliminatingReferences = 0; // 0x0060
    std::uint32_t ArAllowLazyLoading = 0; // 0x0064, UBOOL
    std::uint32_t ArIsObjectReferenceCollector = 0; // 0x0068, UBOOL
    std::uint32_t ArIsCountingMemory = 0; // 0x006C, UBOOL
    std::uint32_t ArPortFlags = 0;   // 0x0070
    std::uint32_t ArShouldSkipBulkData = 0; // 0x0074, UBOOL
    std::uint32_t ArIsSaveGame = 0;  // 0x0078, UBOOL
    std::uint32_t ArIsFinalPackageSave = 0; // 0x007C, UBOOL
    std::int32_t ArMaxSerializeSize = 0; // 0x0080
    std::uint32_t ArIsFilterEditorOnly = 0; // 0x0084, UBOOL
};

struct FLinkerLoadNativeState {
    FArchiveStateMirror ArchiveState; // 0x0000, stock FArchive multiple-inheritance base
    std::uint32_t LoadFlags = 0;     // 0x0088
    std::uint32_t bHaveImportsBeenVerified = 0; // 0x008C, UBOOL
    std::int32_t ExportHash[256]{};  // 0x0090
    TArray<FUntypedBulkData*> BulkDataLoaders; // 0x0490
    void* Loader = nullptr;          // 0x04A0, FArchive*
    std::int32_t NameMapIndex = 0;   // 0x04A8
    std::int32_t ImportMapIndex = 0; // 0x04AC
    std::int32_t ExportMapIndex = 0; // 0x04B0
    std::int32_t DependsMapIndex = 0; // 0x04B4
    std::int32_t ExportHashIndex = 0; // 0x04B8
    std::uint32_t bHasSerializedPackageFileSummary = 0; // 0x04BC, UBOOL
    std::uint32_t bHasFixedUpImportMap = 0; // 0x04C0, UBOOL
    std::uint32_t bHasFoundExistingExports = 0; // 0x04C4, UBOOL
    std::uint32_t bHasFinishedInitialization = 0; // 0x04C8, UBOOL
    std::uint32_t bHasRemappedExternalPackageReferencesForMultilanguageCooks = 0; // 0x04CC, UBOOL
    std::uint32_t bHasIntegratedNamePatches = 0; // 0x04D0, UBOOL
    std::uint32_t bHasIntegratedImportPatches = 0; // 0x04D4, UBOOL
    std::uint32_t bHasIntegratedExportPatches = 0; // 0x04D8, UBOOL
    std::uint32_t bHasIntegratedScriptPatches = 0; // 0x04DC, UBOOL
    std::uint32_t bHasIntegratedDefaultsPatches = 0; // 0x04E0, UBOOL
    std::uint32_t bHasIntegratedEnumPatches = 0; // 0x04E4, UBOOL
    std::uint32_t bHasRemappedExternalPackageReferences = 0; // 0x04E8, UBOOL
    std::uint32_t bIsGatheringDependencies = 0; // 0x04EC, UBOOL
    std::uint32_t bTimeLimitExceeded = 0; // 0x04F0, UBOOL
    std::int32_t IsTimeLimitExceededCallCount = 0; // 0x04F4
    std::uint32_t bUseTimeLimit = 0; // 0x04F8, UBOOL
    float TimeLimit = 0.0f;          // 0x04FC
    double TickStartTime = 0.0;      // 0x0500
    std::uint32_t bFixupExportMapDone = 0; // 0x0508, UBOOL
    std::uint32_t Pad_050C = 0;      // 0x050C
    FPatchReader* PatchDataAr = nullptr; // 0x0510
    void* OriginalLoader = nullptr;  // 0x0518, FArchive*
    TMap<std::int32_t, FScriptPatchData*> FunctionsToPatch; // 0x0520
    TMap<std::int32_t, FPatchData*> DefaultsToPatch; // 0x0570
    TMap<std::int32_t, FEnumPatchData*> EnumsToPatch; // 0x05C0
    UObject* PotentialCrossLevelOwner = nullptr; // 0x0610
    const UProperty* PotentialCrossLevelProperty = nullptr; // 0x0618
    std::uint8_t Pad_0620[0x08]{};   // 0x0620
};

struct FLinkerSaveNativeState {
    FArchiveStateMirror ArchiveState; // 0x0000, stock FArchive multiple-inheritance base
    void* Saver = nullptr;           // 0x0088, FArchive*
    TArray<std::int32_t> ObjectIndices; // 0x0090, PACKAGE_INDEX
    TArray<std::int32_t> NameIndices; // 0x00A0
    std::uint32_t bIsNextObjectSerializePotentialCrossLevelRef = 0; // 0x00B0, UBOOL
    std::uint8_t Pad_00B4[0x0C]{};   // 0x00B4
};

class ULinker : public UObject {
public:
    UPackage* LinkerRoot = nullptr;  // 0x0060
    FPackageFileSummary Summary;     // 0x0068
    TArray<FName> NameMap;           // 0x00F8
    TArray<FObjectImport> ImportMap; // 0x0108
    TArray<FObjectExport> ExportMap; // 0x0118
    TArray<TArray<std::int32_t>> DependsMap; // 0x0128
    TMap<FGuid, std::int32_t> ExportGuidsAwaitingLookup; // 0x0138
    FString Filename;                // 0x0188
    std::uint64_t ContextFlags = 0;  // 0x0198, EObjectFlags
    void* ScriptSHA = nullptr;       // 0x01A0, FSHA1*
    FStructEventMapMirror SerializationPerfTracker; // 0x01A8, stock compile-conditional performance tracker
};

class ULinkerLoad : public ULinker {
public:
    [[nodiscard]] static UClass* StaticClass();
    static ULinkerLoad* CreateLinker(UPackage* parent, const wchar_t* filename, std::uint32_t loadFlags);
    void Detach();
    void Preload(UObject* object);
    void Serialize(void* data, std::int32_t length);
    void Seek(std::int32_t position);
    [[nodiscard]] std::int32_t Tell() const;
    [[nodiscard]] std::int32_t TotalSize() const;
    [[nodiscard]] ULinker* GetLinker() noexcept { return this; }

    FLinkerLoadNativeState NativeLoadState; // 0x01E8, stock FArchive/ULinkerLoad native state to RL size
};

class ULinkerSave : public ULinker {
public:
    [[nodiscard]] static UClass* StaticClass();
    void Detach();
    void Serialize(void* data, std::int32_t length);
    void Seek(std::int32_t position);
    [[nodiscard]] std::int32_t Tell() const;
    [[nodiscard]] ULinker* GetLinker() noexcept { return this; }

    FLinkerSaveNativeState NativeSaveState; // 0x01E8, stock FArchive/ULinkerSave native state to RL size
};

class UPackage : public UObject {
public:
    [[nodiscard]] static UClass* StaticClass();
    [[nodiscard]] std::uint32_t GetPackageFlags() const noexcept { return PackageFlags; }
    [[nodiscard]] bool HasAnyPackageFlags(std::uint32_t flags) const noexcept { return (PackageFlags & flags) != 0; }
    [[nodiscard]] bool IsBound() const noexcept { return bIsBound != 0; }
    [[nodiscard]] float GetLoadTime() const noexcept { return LoadTime; }
    void SetLoadTime(float loadTime) noexcept { LoadTime = loadTime; }

    std::uint32_t bDirty = 0;        // 0x0060, UBOOL
    std::uint32_t bDirtyForPIE = 0;  // 0x0064, UBOOL
    std::uint32_t bHasBeenFullyLoaded = 0; // 0x0068, UBOOL
    std::uint32_t bShouldFindExportsInMemoryFirst = 0; // 0x006C, UBOOL
    std::uint32_t bIsBound = 0;      // 0x0070, UBOOL
    FName FolderName;                // 0x0074
    float LoadTime = 0.0f;           // 0x007C
    FGuid Guid;                      // 0x0080
    std::int32_t FileSize = 0;       // 0x0090
    std::uint32_t Pad_0094 = 0;      // 0x0094
    TArray<UObject*> NetObjects;     // 0x0098
    std::int32_t CurrentNumNetObjects = 0; // 0x00A8
    std::uint32_t Pad_00AC = 0;      // 0x00AC
    TArray<std::int32_t> GenerationNetObjectCount; // 0x00B0
    FName ForcedExportBasePackageName; // 0x00C0
    TMap<FGuid, UObject*> ExportGuids; // 0x00C8
    TArray<FLevelGuids> ImportGuids; // 0x0118
    std::uint32_t PackageFlags = 0;  // 0x0128
    FName FileName;                  // 0x012C
    std::uint32_t Pad_0134 = 0;      // 0x0134
    void* ThumbnailMap = nullptr;    // 0x0138, TScopedPointer<FThumbnailMap>
    std::uint8_t RLPad_0140[0x08]{}; // 0x0140, RL tail to reflected size
};

struct FPackageInfo {
    FName PackageName;               // 0x0000
    UPackage* Parent = nullptr;      // 0x0008
    FGuid Guid;                      // 0x0010
    std::int32_t ObjectBase = 0;     // 0x0020
    std::int32_t ObjectCount = 0;    // 0x0024
    std::int32_t LocalGeneration = 0; // 0x0028
    std::int32_t RemoteGeneration = 0; // 0x002C
    std::uint32_t PackageFlags = 0;  // 0x0030
    FName ForcedExportBasePackageName; // 0x0034
    std::uint8_t LoadingPhase = 0;   // 0x003C
    std::uint8_t Pad_003D[0x03]{};   // 0x003D
    FString Extension;               // 0x0040
    FName FileName;                  // 0x0050
};

class UPackageMap : public UObject {
public:
    [[nodiscard]] static UClass* StaticClass();

    TArray<FPackageInfo> List;       // 0x0060
    TMap<FName, std::int32_t> PackageListMap; // 0x0070
    TMap<UClass*, void*> ClassFieldIndices; // 0x00C0, FClassNetCache*
    std::uint8_t RLPad_0110[0x08]{}; // 0x0110, RL tail to reflected size
};

#if defined(__GNUC__)
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Winvalid-offsetof"
#endif

static_assert(sizeof(FLevelGuids) == 0x18);
static_assert(offsetof(FLevelGuids, Guids) == 0x08);
static_assert(sizeof(FObjectResource) == 0x0C);
static_assert(offsetof(FObjectResource, ObjectName) == 0x00);
static_assert(offsetof(FObjectResource, OuterIndex) == 0x08);
static_assert(sizeof(FObjectImport) == 0x1C);
static_assert(offsetof(FObjectImport, ClassPackage) == 0x0C);
static_assert(offsetof(FObjectImport, ClassName) == 0x14);
static_assert(sizeof(FObjectExport) == 0x68);
static_assert(offsetof(FObjectExport, ClassIndex) == 0x0C);
static_assert(offsetof(FObjectExport, ArchetypeIndex) == 0x14);
static_assert(offsetof(FObjectExport, ScriptSerializationStartOffset) == 0x28);
static_assert(offsetof(FObjectExport, Object) == 0x30);
static_assert(offsetof(FObjectExport, GenerationNetObjectCount) == 0x40);
static_assert(offsetof(FObjectExport, PackageGuid) == 0x50);
static_assert(offsetof(FObjectExport, PackageFlags) == 0x60);
static_assert(sizeof(FStructEventMapMirror) == 0x40);
static_assert(sizeof(FArchiveStateMirror) == 0x88);
static_assert(offsetof(FArchiveStateMirror, ArVer) == 0x08);
static_assert(offsetof(FArchiveStateMirror, ArIsLoading) == 0x14);
static_assert(offsetof(FArchiveStateMirror, ArContainsCookedData) == 0x40);
static_assert(offsetof(FArchiveStateMirror, ArPortFlags) == 0x70);
static_assert(offsetof(FArchiveStateMirror, ArIsFilterEditorOnly) == 0x84);
static_assert(sizeof(FLinkerLoadNativeState) == 0x628);
static_assert(offsetof(FLinkerLoadNativeState, ArchiveState) == 0x00);
static_assert(offsetof(FLinkerLoadNativeState, LoadFlags) == 0x88);
static_assert(offsetof(FLinkerLoadNativeState, ExportHash) == 0x90);
static_assert(offsetof(FLinkerLoadNativeState, BulkDataLoaders) == 0x490);
static_assert(offsetof(FLinkerLoadNativeState, Loader) == 0x4A0);
static_assert(offsetof(FLinkerLoadNativeState, NameMapIndex) == 0x4A8);
static_assert(offsetof(FLinkerLoadNativeState, bHasSerializedPackageFileSummary) == 0x4BC);
static_assert(offsetof(FLinkerLoadNativeState, TickStartTime) == 0x500);
static_assert(offsetof(FLinkerLoadNativeState, PatchDataAr) == 0x510);
static_assert(offsetof(FLinkerLoadNativeState, FunctionsToPatch) == 0x520);
static_assert(offsetof(FLinkerLoadNativeState, DefaultsToPatch) == 0x570);
static_assert(offsetof(FLinkerLoadNativeState, EnumsToPatch) == 0x5C0);
static_assert(offsetof(FLinkerLoadNativeState, PotentialCrossLevelOwner) == 0x610);
static_assert(sizeof(FLinkerSaveNativeState) == 0xC0);
static_assert(offsetof(FLinkerSaveNativeState, ArchiveState) == 0x00);
static_assert(offsetof(FLinkerSaveNativeState, Saver) == 0x88);
static_assert(offsetof(FLinkerSaveNativeState, ObjectIndices) == 0x90);
static_assert(offsetof(FLinkerSaveNativeState, NameIndices) == 0xA0);
static_assert(offsetof(FLinkerSaveNativeState, bIsNextObjectSerializePotentialCrossLevelRef) == 0xB0);
static_assert(sizeof(FPackageFileSummary) == 0x90);
static_assert(offsetof(FPackageFileSummary, Guid) == 0x28);
static_assert(offsetof(FPackageFileSummary, Generations) == 0x38);
static_assert(offsetof(FPackageFileSummary, CompressedChunks) == 0x60);
static_assert(offsetof(FPackageFileSummary, AdditionalPackagesToCook) == 0x78);
static_assert(sizeof(ULinker) == 0x1E8);
static_assert(offsetof(ULinker, LinkerRoot) == 0x60);
static_assert(offsetof(ULinker, Summary) == 0x68);
static_assert(offsetof(ULinker, NameMap) == 0xF8);
static_assert(offsetof(ULinker, ExportMap) == 0x118);
static_assert(offsetof(ULinker, ExportGuidsAwaitingLookup) == 0x138);
static_assert(offsetof(ULinker, Filename) == 0x188);
static_assert(offsetof(ULinker, ContextFlags) == 0x198);
static_assert(offsetof(ULinker, ScriptSHA) == 0x1A0);
static_assert(offsetof(ULinker, SerializationPerfTracker) == 0x1A8);
static_assert(sizeof(ULinkerLoad) == 0x810);
static_assert(offsetof(ULinkerLoad, NativeLoadState) == 0x1E8);
static_assert(sizeof(ULinkerSave) == 0x2A8);
static_assert(offsetof(ULinkerSave, NativeSaveState) == 0x1E8);
static_assert(sizeof(UPackage) == 0x148);
static_assert(offsetof(UPackage, bDirty) == 0x60);
static_assert(offsetof(UPackage, FolderName) == 0x74);
static_assert(offsetof(UPackage, Guid) == 0x80);
static_assert(offsetof(UPackage, NetObjects) == 0x98);
static_assert(offsetof(UPackage, GenerationNetObjectCount) == 0xB0);
static_assert(offsetof(UPackage, ExportGuids) == 0xC8);
static_assert(offsetof(UPackage, ImportGuids) == 0x118);
static_assert(offsetof(UPackage, PackageFlags) == 0x128);
static_assert(offsetof(UPackage, FileName) == 0x12C);
static_assert(offsetof(UPackage, ThumbnailMap) == 0x138);
static_assert(sizeof(FPackageInfo) == 0x58);
static_assert(offsetof(FPackageInfo, Parent) == 0x08);
static_assert(offsetof(FPackageInfo, Guid) == 0x10);
static_assert(offsetof(FPackageInfo, ForcedExportBasePackageName) == 0x34);
static_assert(offsetof(FPackageInfo, Extension) == 0x40);
static_assert(offsetof(FPackageInfo, FileName) == 0x50);
static_assert(sizeof(UPackageMap) == 0x118);
static_assert(offsetof(UPackageMap, List) == 0x60);
static_assert(offsetof(UPackageMap, PackageListMap) == 0x70);
static_assert(offsetof(UPackageMap, ClassFieldIndices) == 0xC0);

#if defined(__GNUC__)
#pragma GCC diagnostic pop
#endif

} // namespace Kuru::SDK
