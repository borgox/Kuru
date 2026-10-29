#pragma once

#include <Core/Containers.hpp>
#include <Core/Delegates.hpp>
#include <Core/Math.hpp>
#include <Core/Misc.hpp>
#include <Core/ObjectModel.hpp>
#include <Core/PropertyTag.hpp>
#include <Core/Strings.hpp>

#include <array>
#include <cstdint>

namespace Kuru::SDK {

// Engine delegates occupy 32 reflected bytes in the build verified on 2026-10-29.
// Their invocation layout is not modeled; keep the storage opaque.
using FEngineDelegateStorage = std::array<std::uint8_t, 0x20>;
static_assert(sizeof(FEngineDelegateStorage) == 0x20);

class AActor;
class ALevelGridVolume;
class UModel;
class UModelComponent;
class UPrimitiveComponent;
class USequence;
class AWorldInfo;
class UFaceFXAnimSet;
class UFont;
class UScriptViewportClient;
class USubsystem;
class UEngine;
class ULevel;
class ULevelBase;
class UWorld;
class UGameEngine;
class UGameViewportClient;
class UDEPRECATED_SaveGameSummary;
class UNetDriver;
class UDemoRecDriver;
class ULineBatchComponent;
class URB_BodyInstance;
class URB_ConstraintInstance;
class UAnimTree;
class ULightComponent;
class ULightEnvironmentComponent;
class UDominantDirectionalLightComponent;
class UDominantSpotLightComponent;
class UDominantPointLightComponent;
class UStaticMesh;
class ANavigationPoint;
class ACoverLink;
class APylon;

struct FSceneInterface;
struct FPrimitiveHashBase;
struct FNavigationOctree;
struct FCrowdAttractorOctreeType;
struct FNavMeshWorld;
struct FRBPhysScene;
struct FNxDebugRenderer;
struct FObserverInterface;
struct FWorldGCReferencer;

struct FShowFlagsMirror {
    std::uint64_t Flags0 = 0;        // 0x0000
    std::uint64_t Flags1 = 0;        // 0x0008

    [[nodiscard]] bool HasFlag(std::uint32_t bitIndex) const noexcept {
        if (bitIndex >= 128) {
            return false;
        }
        return bitIndex < 64 ? ((Flags0 & (1ull << bitIndex)) != 0) : ((Flags1 & (1ull << (bitIndex - 64))) != 0);
    }
};

struct FExportShowFlagsMirror : FShowFlagsMirror {
};

struct FTitleSafeZoneArea {
    float MaxPercentX = 0.0f;
    float MaxPercentY = 0.0f;
    float RecommendedPercentX = 0.0f;
    float RecommendedPercentY = 0.0f;
};

struct FURLMirror {
    FString Protocol;                // 0x0000
    FString Host;                    // 0x0010
    std::int32_t Port;               // 0x0020
    std::uint32_t Pad_0024;          // 0x0024
    FString Map;                     // 0x0028
    TArray<FString> Op;              // 0x0038
    FString Portal;                  // 0x0048
    std::int32_t Valid;              // 0x0058
    std::uint32_t Pad_005C;          // 0x005C
};

struct FLevelViewportInfo {
    FVector CamPosition;             // 0x0000
    FRotator CamRotation;            // 0x000C
    float CamOrthoZoom = 0.0f;       // 0x0018
};

struct FCachedPhysSMData {
    FVector Scale3D;                 // 0x0000
    std::int32_t CachedDataIndex = 0; // 0x000C
};

using FCachedPerTriPhysSMData = FCachedPhysSMData;

struct FKCachedConvexDataElement {
    TArray<std::uint8_t> ConvexElementData; // 0x0000
};

struct FKCachedConvexData {
    TArray<FKCachedConvexDataElement> CachedConvexElements; // 0x0000
};

struct FKCachedPerTriData {
    TArray<std::uint8_t> CachedPerTriData; // 0x0000
};

struct FKSphereElem {
    FMatrix TM;                      // 0x0000
    float Radius = 0.0f;             // 0x0040
    std::uint32_t CollisionFlags = 0; // 0x0044, bNoRBCollision/bPerPolyShape bitfield
};

struct FKBoxElem {
    FMatrix TM;                      // 0x0000
    float X = 0.0f;                  // 0x0040
    float Y = 0.0f;                  // 0x0044
    float Z = 0.0f;                  // 0x0048
    std::uint32_t CollisionFlags = 0; // 0x004C, bNoRBCollision/bPerPolyShape bitfield
};

struct FKSphylElem {
    FMatrix TM;                      // 0x0000
    float Radius = 0.0f;             // 0x0040
    float Length = 0.0f;             // 0x0044
    std::uint32_t CollisionFlags = 0; // 0x0048, bNoRBCollision/bPerPolyShape bitfield
    std::uint32_t Pad_004C = 0;      // 0x004C
};

struct FKConvexElem {
    TArray<FVector> VertexData;      // 0x0000
    TArray<FPlane> PermutedVertexData; // 0x0010
    TArray<std::int32_t> FaceTriData; // 0x0020
    TArray<FVector> EdgeDirections;  // 0x0030
    TArray<FVector> FaceNormalDirections; // 0x0040
    TArray<FPlane> FacePlaneData;    // 0x0050
    FBox ElemBox;                    // 0x0060
    std::uint32_t Pad_007C = 0;      // 0x007C
};

struct FKAggregateGeom {
    TArray<FKSphereElem> SphereElems; // 0x0000
    TArray<FKBoxElem> BoxElems;      // 0x0010
    TArray<FKSphylElem> SphylElems;  // 0x0020
    TArray<FKConvexElem> ConvexElems; // 0x0030
    void* RenderInfo = nullptr;      // 0x0040, FKConvexGeomRenderInfo*
    std::uint32_t bSkipCloseAndParallelChecks = 0; // 0x0048, BITFIELD storage
    std::uint32_t Pad_004C = 0;      // 0x004C
};

struct FPrecomputedVisibilityCell {
    FVector Min;                     // 0x0000
    std::uint16_t ChunkIndex = 0;    // 0x000C
    std::uint16_t DataOffset = 0;    // 0x000E
};

struct FCompressedVisibilityChunk {
    std::uint32_t bCompressed = 0;   // 0x0000, UBOOL
    std::int32_t UncompressedSize = 0; // 0x0004
    TArray<std::uint8_t> Data;       // 0x0008
};

struct FPrecomputedVisibilityBucket {
    std::int32_t CellDataSize = 0;   // 0x0000
    std::uint32_t Pad_0004 = 0;      // 0x0004
    TArray<FPrecomputedVisibilityCell> Cells; // 0x0008
    TArray<FCompressedVisibilityChunk> CellDataChunks; // 0x0018
};

struct FPrecomputedVisibilityHandler {
    FVector2D PrecomputedVisibilityCellBucketOriginXY; // 0x0000
    float PrecomputedVisibilityCellSizeXY = 0.0f; // 0x0008
    float PrecomputedVisibilityCellSizeZ = 0.0f; // 0x000C
    std::int32_t PrecomputedVisibilityCellBucketSizeXY = 0; // 0x0010
    std::int32_t PrecomputedVisibilityNumCellBuckets = 0; // 0x0014
    std::int32_t Id = 0;            // 0x0018
    std::uint32_t Pad_001C = 0;     // 0x001C
    TArray<FPrecomputedVisibilityBucket> PrecomputedVisibilityCellBuckets; // 0x0020
};

struct FPrecomputedVolumeDistanceField {
    float VolumeMaxDistance = 0.0f; // 0x0000
    FBox VolumeBox;                 // 0x0004
    std::int32_t VolumeSizeX = 0;   // 0x0020
    std::int32_t VolumeSizeY = 0;   // 0x0024
    std::int32_t VolumeSizeZ = 0;   // 0x0028
    std::uint32_t Pad_002C = 0;     // 0x002C
    TArray<FColor> Data;            // 0x0030
};

struct FGuidPair {
    FGuid Guid;                     // 0x0000
    std::uint32_t RefId = 0;        // 0x0010
    std::uint32_t Pad_0014 = 0;     // 0x0014
};

struct FCoverIndexPair {
    std::uint32_t ActorRefItem = 0; // 0x0000
    std::uint8_t SlotIdx = 0;       // 0x0004
    std::uint8_t Pad_0005[0x03]{};  // 0x0005
};

struct FRenderCommandFence {
    std::uint32_t NumPendingFences = 0; // 0x0000
};

struct FLevelNativeState {
    std::int32_t SceneIndex = 0;       // 0x0000
    std::uint32_t Pad_0004 = 0;        // 0x0004
    void* LevelBSPPhysMesh = nullptr;  // 0x0008, NxTriangleMesh*
    void* LevelBSPActor = nullptr;     // 0x0010, NxActor*
    void* LevelConvexBSPActor = nullptr; // 0x0018, NxActor*
    TArray<std::uint8_t> CachedPhysBSPData; // 0x0020
    std::int32_t CachedPhysBSPDataVersion = 0; // 0x0030
    std::uint32_t Pad_0034 = 0;        // 0x0034
    TMultiMap<UStaticMesh*, FCachedPhysSMData> CachedPhysSMDataMap; // 0x0038
    std::int32_t CachedPhysSMDataVersion = 0; // 0x0088
    std::uint32_t Pad_008C = 0;        // 0x008C
    TArray<FKCachedConvexData> CachedPhysSMDataStore; // 0x0090
    TMultiMap<UStaticMesh*, FCachedPerTriPhysSMData> CachedPhysPerTriSMDataMap; // 0x00A0
    TArray<FKCachedPerTriData> CachedPhysPerTriSMDataStore; // 0x00F0
    FKAggregateGeom ConvexBSPAggGeom;  // 0x0100
    FKCachedConvexData CachedPhysConvexBSPData; // 0x0150
    std::int32_t CachedPhysConvexBSPVersion = 0; // 0x0160
    std::uint32_t Pad_0164 = 0;        // 0x0164
    ANavigationPoint* NavListStart = nullptr; // 0x0168
    ANavigationPoint* NavListEnd = nullptr; // 0x0170
    ACoverLink* CoverListStart = nullptr; // 0x0178
    ACoverLink* CoverListEnd = nullptr; // 0x0180
    APylon* PylonListStart = nullptr;  // 0x0188
    APylon* PylonListEnd = nullptr;    // 0x0190
    TArray<FGuidPair> CrossLevelCoverGuidRefs; // 0x0198
    TArray<ACoverLink*> CoverLinkRefs; // 0x01A8
    TArray<FCoverIndexPair> CoverIndexPairs; // 0x01B8
    TArray<AActor*> TickableActors;    // 0x01C8
    TArray<AActor*> PendingUntickableActors; // 0x01D8
    TArray<AActor*> CrossLevelActors;  // 0x01E8
    void* PrecomputedLightVolume = nullptr; // 0x01F8, FPrecomputedLightVolume*
    FPrecomputedVisibilityHandler PrecomputedVisibilityHandler; // 0x0200
    FPrecomputedVolumeDistanceField PrecomputedVolumeDistanceField; // 0x0230
    FRenderCommandFence RemoveFromSceneFence; // 0x0270
    std::uint32_t Pad_0274 = 0;        // 0x0274
    FMatrix AppliedLevelTransform;     // 0x0278
    std::uint32_t bAreComponentsCurrentlyAttached = 0; // 0x02B8, UBOOL
    std::uint32_t bGeometryDirtyForLighting = 0; // 0x02BC, UBOOL
    std::uint32_t bHasVisibilityRequestPending = 0; // 0x02C0, UBOOL
    std::uint32_t bAlreadyMovedActors = 0; // 0x02C4, UBOOL
    std::uint32_t bAlreadyUpdatedComponents = 0; // 0x02C8, UBOOL
    std::uint32_t bAlreadyAssociatedStreamableResources = 0; // 0x02CC, UBOOL
    std::uint32_t bAlreadyInitializedActors = 0; // 0x02D0, UBOOL
    std::uint32_t bAlreadyRoutedActorBeginPlay = 0; // 0x02D4, UBOOL
    std::uint32_t bAlreadyFixedUpCrossLevelRefs = 0; // 0x02D8, UBOOL
    std::uint32_t bAlreadyRoutedSequenceBeginPlay = 0; // 0x02DC, UBOOL
    std::uint32_t bAlreadySortedActorList = 0; // 0x02E0, UBOOL
    std::int32_t CurrentActorIndexForUpdateComponents = 0; // 0x02E4
    std::uint32_t bAlreadyCreateBSPPhysMesh = 0; // 0x02E8, UBOOL
    std::uint32_t bAlreadyInitializedAllActorRBPhys = 0; // 0x02EC, UBOOL
    std::int32_t CurrentActorIndexForInitActorsRBPhys = 0; // 0x02F0
    std::uint8_t Pad_02F4[0x0C]{};     // 0x02F4
};

struct FWorldGCReferencer {
    FPointer VfTable;                      // 0x0000, native vtable in RL module
    FPointer Pad_0008;                     // 0x0008
    UGameViewportClient* ViewportClient;     // 0x0010, probed GameViewportClient_TA in Woods_Night_P match
    FPointer Pad_0018;                     // 0x0018
    float ReferenceScaleX = 0.0f;          // 0x0020, runtime-probed 1.0f native referencer scalar
    float ReferenceScaleY = 0.0f;          // 0x0024, runtime-probed 1.0f native referencer scalar
    UObject* PostProcessChain;             // 0x0028, probed Transient.PostProcessChain in match referencer
};

struct FWorldNativeTailState {
    FPointer UnusedNativePointer_0370;     // 0x0000, probed null in menu + Woods_Night_P match worlds
    FPointer UnusedNativePointer_0378;     // 0x0008
    FPointer UnusedNativePointer_0380;     // 0x0010
    TArray<UObject*> TutorialInstances;    // 0x0018, empty in main menu + active match
    FPointer UnusedNativePointer_0398;     // 0x0028, probed null when GCReferencers is populated
    std::uint32_t bHasGCReferencers = 0;   // 0x0030, UBOOL; set when GCReferencers is populated
    std::uint32_t Pad_0034 = 0;          // 0x0034
    TArray<FWorldGCReferencer*> GCReferencers; // 0x0038, native GC root holders. match count=1 with process-stable referencer
};

class ULevelBase : public UObject {
public:
    [[nodiscard]] static UClass* StaticClass();

    TTransArray<AActor*> Actors;     // 0x0060
    FURLMirror URL;                  // 0x0078
};

class ULevel : public ULevelBase {
public:
    [[nodiscard]] static UClass* StaticClass();

    UModel* Model;                   // 0x00D8
    TArray<UModelComponent*> ModelComponents; // 0x00E0
    TArray<USequence*> GameSequences; // 0x00F0
    TMap<UObject*, TArray<std::uint8_t>> TextureToInstancesMap; // 0x0100, stock texture-streaming map mirror
    TMap<UPrimitiveComponent*, TArray<std::uint8_t>> DynamicTextureInstances; // 0x0150
    TMap<UObject*, std::uint32_t> ForceStreamTextures; // 0x01A0, UBOOL values
    std::int32_t iFirstNetRelevantActor; // 0x01F0
    std::int32_t iFirstDynamicActor; // 0x01F4
    float LightmapTotalSize;         // 0x01F8
    float ShadowmapTotalSize;        // 0x01FC
    FLevelNativeState NativeState;   // 0x0200, stock physics/navigation/precomputed visibility/tick state
};

class UWorld : public UObject {
public:
    [[nodiscard]] static UClass* StaticClass();

    FPointer VfTable_FNetworkNotify; // 0x0060, stock native FNetworkNotify secondary vtable
    FPointer RLPreSceneLayoutSlot;   // 0x0068, RL +8B layout shift vs stock Scene@0x68 null in menu + match
    FSceneInterface* Scene;          // 0x0070, native, concrete impl stores owning UWorld* at Scene+0x8
    TArray<ULevel*> Levels;          // 0x0078
    ULevel* PersistentLevel;         // 0x0088
    UFaceFXAnimSet* PersistentFaceFXAnimSet; // 0x0090
    ULevel* CurrentLevel;            // 0x0098
    ULevel* CurrentLevelPendingVisibility; // 0x00A0
    ALevelGridVolume* CurrentLevelGridVolume; // 0x00A8
    FLevelViewportInfo EditorViews[4]; // 0x00B0
    UDEPRECATED_SaveGameSummary* SaveGameSummary_DEPRECATED; // 0x0120
    UNetDriver* NetDriver;           // 0x0128
    FURLMirror URL;                  // 0x0130
    UDemoRecDriver* DemoRecDriver;   // 0x0190
    UNetDriver* PeerNetDriver;       // 0x0198
    FPrimitiveHashBase* Hash;        // 0x01A0
    FPrimitiveHashBase* BackupHash;  // 0x01A8
    FNavigationOctree* NavigationOctree; // 0x01B0
    FCrowdAttractorOctreeType* CrowdAttractorOctree; // 0x01B8
    FNavMeshWorld* NavMeshWorld;     // 0x01C0
    TArray<AActor*> NewlySpawned;    // 0x01C8
    std::uint32_t InTick;            // 0x01D8, UBOOL
    std::uint32_t Ticked;            // 0x01DC, UBOOL
    std::int32_t TickGroup;          // 0x01E0, ETickingGroup
    std::uint32_t bPostTickComponentUpdate; // 0x01E4, UBOOL
    std::int32_t NetTag;             // 0x01E8
    std::int32_t PlayerNum;          // 0x01EC
    ULineBatchComponent* LineBatcher; // 0x01F0
    ULineBatchComponent* PersistentLineBatcher; // 0x01F8
    std::uint32_t bShowLineChecks;   // 0x0200, UBOOL
    std::uint32_t bShowExtentLineChecks; // 0x0204, UBOOL
    std::uint32_t bShowPointChecks;  // 0x0208, UBOOL
    std::uint32_t Pad_020C;          // 0x020C
    FRBPhysScene* RBPhysScene;       // 0x0210
    FNxDebugRenderer* DebugRenderer; // 0x0218
    TArray<URB_BodyInstance*> BodyInstancePool; // 0x0220
    TArray<URB_ConstraintInstance*> ConstraintInstancePool; // 0x0230
    TArray<UAnimTree*> AnimTreePool; // 0x0240
    float TimeSinceLastPendingKillPurge; // 0x0250
    std::uint32_t FullPurgeTriggered; // 0x0254, UBOOL
    std::uint32_t bShouldDelayGarbageCollect; // 0x0258, UBOOL
    std::uint32_t bIsWorldInitialized; // 0x025C, UBOOL
    TSparseArray<ULightComponent*> StaticLightList; // 0x0260
    TSparseArray<ULightComponent*> DynamicLightList; // 0x0288
    TSparseArray<ULightEnvironmentComponent*> LightEnvironmentList; // 0x02B0
    UDominantDirectionalLightComponent* DominantDirectionalLight; // 0x02D8
    TSparseArray<UDominantSpotLightComponent*> DominantSpotLights; // 0x02E0
    TSparseArray<UDominantPointLightComponent*> DominantPointLights; // 0x0308
    std::int32_t AllowLevelLoadOverride; // 0x0330
    std::uint32_t bDisallowRoutingSequenceBeginPlay; // 0x0334, UBOOL
    std::int32_t StreamingVolumeUpdateDelay; // 0x0338
    std::uint32_t Pad_033C;          // 0x033C
    TArray<UObject*> ExtraReferencedObjects; // 0x0340
    std::uint32_t bIsLevelStreamingFrozen; // 0x0350, UBOOL
    std::uint32_t bDisableCrowds;    // 0x0354, UBOOL
    std::uint32_t bAllowDecalAttach; // 0x0358, UBOOL
    std::uint32_t bDoDelayedUpdateCullDistanceVolumes; // 0x035C, UBOOL
    TArray<FObserverInterface*> Observers; // 0x0360
    FWorldNativeTailState NativeTailState; // 0x0370, RL/stock world native tail to reflected size
};

class USubsystem : public UObject {
public:
    [[nodiscard]] static UClass* StaticClass();

    FPointer VfTable_FExec;          // 0x0060
};

class UScriptViewportClient : public UObject {
public:
    [[nodiscard]] static UClass* StaticClass();

    FPointer VfTable_FViewportClient; // 0x0060
};

class UEngine : public USubsystem {
public:
    [[nodiscard]] static UClass* StaticClass();

    FString GameShareClassName;      // 0x0068
    UClass* GameShareClass;          // 0x0078
    FString EngineShareClassName;    // 0x0080
    UObject* EngineShare;            // 0x0090
    std::int32_t BuildID;            // 0x0098
    std::uint32_t Pad_009C;          // 0x009C
    UObject* NetDriverSecurity;      // 0x00A0
    UFont* TinyFont;                 // 0x00A8
    FString TinyFontName;            // 0x00B0
    UFont* SmallFont;                // 0x00C0
    FString SmallFontName;           // 0x00C8
    UFont* MediumFont;               // 0x00D8
    FString MediumFontName;          // 0x00E0
    UFont* LargeFont;                // 0x00F0
    FString LargeFontName;           // 0x00F8
    UObject* SubtitleFont;           // 0x0108
    FString SubtitleFontName;        // 0x0110
    UObject* ScalableFont;           // 0x0120
    FString ScalableFontName;        // 0x0128
    TArray<UObject*> AdditionalFonts; // 0x0138
    TArray<FString> AdditionalFontNames; // 0x0148
    UClass* ConsoleClass;            // 0x0158
    FString ConsoleClassName;        // 0x0160
    UClass* GameViewportClientClass; // 0x0170
    FString GameViewportClientClassName; // 0x0178
    UClass* DataStoreClientClass;    // 0x0188
    FString DataStoreClientClassName; // 0x0190
    UClass* LocalPlayerClass;        // 0x01A0
    FString LocalPlayerClassName;    // 0x01A8
    UObject* DefaultMaterial;        // 0x01B8
    FString DefaultMaterialName;     // 0x01C0
    UObject* DefaultDecalMaterial;   // 0x01D0
    FString DefaultDecalMaterialName; // 0x01D8
    UObject* DefaultTexture;         // 0x01E8
    FString DefaultTextureName;      // 0x01F0
    UObject* WireframeMaterial;      // 0x0200
    FString WireframeMaterialName;   // 0x0208
    UObject* EmissiveTexturedMaterial; // 0x0218
    FString EmissiveTexturedMaterialName; // 0x0220
    UObject* GeomMaterial;           // 0x0230
    FString GeomMaterialName;        // 0x0238
    UObject* DefaultFogVolumeMaterial; // 0x0248
    FString DefaultFogVolumeMaterialName; // 0x0250
    UObject* TickMaterial;           // 0x0260
    FString TickMaterialName;        // 0x0268
    UObject* CrossMaterial;          // 0x0278
    FString CrossMaterialName;       // 0x0280
    UObject* LevelColorationLitMaterial; // 0x0290
    FString LevelColorationLitMaterialName; // 0x0298
    UObject* LevelColorationUnlitMaterial; // 0x02A8
    FString LevelColorationUnlitMaterialName; // 0x02B0
    UObject* LightingTexelDensityMaterial; // 0x02C0
    FString LightingTexelDensityName; // 0x02C8
    UObject* ShadedLevelColorationLitMaterial; // 0x02D8
    FString ShadedLevelColorationLitMaterialName; // 0x02E0
    UObject* ShadedLevelColorationUnlitMaterial; // 0x02F0
    FString ShadedLevelColorationUnlitMaterialName; // 0x02F8
    UObject* RemoveSurfaceMaterial;  // 0x0308
    FString RemoveSurfaceMaterialName; // 0x0310
    UObject* VertexColorMaterial;    // 0x0320
    FString VertexColorMaterialName; // 0x0328
    UObject* VertexColorViewModeMaterial_ColorOnly; // 0x0338
    FString VertexColorViewModeMaterialName_ColorOnly; // 0x0340
    UObject* VertexColorViewModeMaterial_AlphaAsColor; // 0x0350
    FString VertexColorViewModeMaterialName_AlphaAsColor; // 0x0358
    UObject* VertexColorViewModeMaterial_RedOnly; // 0x0368
    FString VertexColorViewModeMaterialName_RedOnly; // 0x0370
    UObject* VertexColorViewModeMaterial_GreenOnly; // 0x0380
    FString VertexColorViewModeMaterialName_GreenOnly; // 0x0388
    UObject* VertexColorViewModeMaterial_BlueOnly; // 0x0398
    FString VertexColorViewModeMaterialName_BlueOnly; // 0x03A0
    UObject* HeatmapMaterial;        // 0x03B0
    FString HeatmapMaterialName;     // 0x03B8
    UObject* BoneWeightMaterial;     // 0x03C8
    FString BoneWeightMaterialName;  // 0x03D0
    UObject* TangentColorMaterial;   // 0x03E0
    FString TangentColorMaterialName; // 0x03E8
    UObject* MobileEmulationMasterMaterial; // 0x03F8
    FString MobileEmulationMasterMaterialName; // 0x0400
    UObject* ProcBuildingSimpleMaterial; // 0x0410
    FString ProcBuildingSimpleMaterialName; // 0x0418
    UObject* BuildingQuadStaticMesh; // 0x0428
    FString BuildingQuadStaticMeshName; // 0x0430
    float ProcBuildingLODColorTexelsPerWorldUnit; // 0x0440
    float ProcBuildingLODLightingTexelsPerWorldUnit; // 0x0444
    std::int32_t MaxProcBuildingLODColorTextureSize; // 0x0448
    std::int32_t MaxProcBuildingLODLightingTextureSize; // 0x044C
    std::uint32_t UseProcBuildingLODTextureCropping : 1; // 0x0450
    std::uint32_t ForcePowerOfTwoProcBuildingLODTextures : 1;
    std::uint32_t bCombineSimilarMappings : 1;
    std::uint32_t bRenderLightMapDensityGrayscale : 1;
    std::uint32_t bScreenshotRequested : 1;
    std::uint32_t bUseSound : 1;
    std::uint32_t bUseBackgroundLevelStreaming : 1;
    std::uint32_t bSubtitlesEnabled : 1;
    std::uint32_t bSubtitlesForcedOff : 1;
    std::uint32_t bSmoothFrameRate : 1;
    std::uint32_t bCheckForMultiplePawnsSpawnedInAFrame : 1;
    std::uint32_t bShouldGenerateSimpleLightmaps : 1;
    std::uint32_t bForceStaticTerrain : 1;
    std::uint32_t bForceCPUSkinning : 1;
    std::uint32_t bUsePostProcessEffects : 1;
    std::uint32_t bOnScreenKismetWarnings : 1;
    std::uint32_t bEnableKismetLogging : 1;
    std::uint32_t bAllowMatureLanguage : 1;
    std::uint32_t bDisablePhysXHardwareSupport : 1;
    std::uint32_t bEnablePhysX : 1;
    std::uint32_t bPauseOnLossOfFocus : 1;
    std::uint32_t bCheckParticleRenderSize : 1;
    std::uint32_t bEnableColorClear : 1;
    std::uint32_t bEnableSwitchRenderMode : 1;
    std::uint32_t bShowAllHiddenObjects : 1;
    std::uint32_t bAreConstraintsDirty : 1;
    std::uint32_t bHasPendingGlobalReattach : 1;
    std::uint32_t bEnableOnScreenDebugMessages : 1;
    std::uint32_t bEnableOnScreenDebugMessagesDisplay : 1;
    std::uint32_t bSuppressMapWarnings : 1;
    std::uint32_t bCookSeparateSharedMPGameContent : 1;
    std::uint32_t bUseRecastNavMesh : 1;
    std::uint32_t bDisableAILogging : 1; // 0x0454
    std::uint32_t bUseNormalMapsForSimpleLightMaps : 1;
    std::uint32_t bStartWithMatineeCapture : 1;
    std::uint32_t bCompressMatineeCapture : 1;
    std::uint32_t bLockReadOnlyLevels : 1;
    std::uint32_t PadBits_0454 : 27;
    float MaxRMSDForCombiningMappings; // 0x0458
    std::int32_t ImageReflectionTextureSize; // 0x045C
    FLinearColor LightingOnlyBrightness; // 0x0460
    TArray<FLinearColor> LightComplexityColors; // 0x0470
    TArray<FLinearColor> ShaderComplexityColors; // 0x0480
    float MaxPixelShaderAdditiveComplexityCount; // 0x0490
    float MinTextureDensity;        // 0x0494
    float IdealTextureDensity;      // 0x0498
    float MaxTextureDensity;        // 0x049C
    float MinLightMapDensity;       // 0x04A0
    float IdealLightMapDensity;     // 0x04A4
    float MaxLightMapDensity;       // 0x04A8
    float RenderLightMapDensityGrayscaleScale; // 0x04AC
    float RenderLightMapDensityColorScale; // 0x04B0
    FLinearColor LightMapDensityVertexMappedColor; // 0x04B4
    FLinearColor LightMapDensitySelectedColor; // 0x04C4
    std::uint32_t Pad_04D4;         // 0x04D4
    TArray<std::uint8_t> StatColorMappings; // 0x04D8
    UObject* EditorBrushMaterial;   // 0x04E8
    FString EditorBrushMaterialName; // 0x04F0
    UObject* DefaultPhysMaterial;   // 0x0500
    FString DefaultPhysMaterialName; // 0x0508
    UObject* LandscapeHolePhysMaterial; // 0x0518
    FString LandscapeHolePhysMaterialName; // 0x0520
    UObject* ApexDamageParams;      // 0x0530
    FString ApexDamageParamsName;   // 0x0538
    UObject* TerrainErrorMaterial;  // 0x0548
    FString TerrainErrorMaterialName; // 0x0550
    std::int32_t TerrainMaterialMaxTextureCount; // 0x0560
    std::int32_t TerrainTessellationCheckCount; // 0x0564
    float TerrainTessellationCheckDistance; // 0x0568
    std::uint32_t Pad_056C;         // 0x056C
    UClass* OnlineSubsystemClass;   // 0x0570
    FString DefaultOnlineSubsystemName; // 0x0578
    UObject* DefaultPostProcess;    // 0x0588
    FString DefaultPostProcessName; // 0x0590
    UObject* ThumbnailSkeletalMeshPostProcess; // 0x05A0
    FString ThumbnailSkeletalMeshPostProcessName; // 0x05A8
    UObject* ThumbnailParticleSystemPostProcess; // 0x05B8
    FString ThumbnailParticleSystemPostProcessName; // 0x05C0
    UObject* ThumbnailMaterialPostProcess; // 0x05D0
    FString ThumbnailMaterialPostProcessName; // 0x05D8
    UObject* DefaultUIScenePostProcess; // 0x05E8
    FString DefaultUIScenePostProcessName; // 0x05F0
    UObject* DefaultUICaretMaterial; // 0x0600
    FString DefaultUICaretMaterialName; // 0x0608
    UObject* SceneCaptureReflectActorMaterial; // 0x0618
    FString SceneCaptureReflectActorMaterialName; // 0x0620
    UObject* SceneCaptureCubeActorMaterial; // 0x0630
    FString SceneCaptureCubeActorMaterialName; // 0x0638
    UObject* ScreenDoorNoiseTexture; // 0x0648
    FString ScreenDoorNoiseTextureName; // 0x0650
    UObject* ImageGrainNoiseTexture; // 0x0660
    FString ImageGrainNoiseTextureName; // 0x0668
    UObject* RandomAngleTexture;     // 0x0678
    FString RandomAngleTextureName;  // 0x0680
    UObject* RandomNormalTexture;    // 0x0690
    FString RandomNormalTextureName; // 0x0698
    UObject* RandomMirrorDiscTexture; // 0x06A8
    FString RandomMirrorDiscTextureName; // 0x06B0
    UObject* WeightMapPlaceholderTexture; // 0x06C0
    FString WeightMapPlaceholderTextureName; // 0x06C8
    UObject* LightMapDensityTexture; // 0x06D8
    FString LightMapDensityTextureName; // 0x06E0
    UObject* SMAAAreaTexture;        // 0x06F0
    FString SMAAAreaTextureName;     // 0x06F8
    UObject* SMAASearchTexture;      // 0x0708
    FString SMAASearchTextureName;   // 0x0710
    UObject* LightMapDensityNormal;  // 0x0720
    FString LightMapDensityNormalName; // 0x0728
    UObject* DefaultSound;           // 0x0738
    FString DefaultSoundName;        // 0x0740
    float TimeBetweenPurgingPendingKillObjects; // 0x0750
    std::uint32_t Pad_0754;          // 0x0754
    UObject* Client;                 // 0x0758
    TArray<UObject*> GamePlayers;    // 0x0760
    UObject* GameViewport;           // 0x0770
    TArray<FString> DeferredCommands; // 0x0778
    std::int32_t TickCycles;         // 0x0788
    std::int32_t GameCycles;         // 0x078C
    std::int32_t ClientCycles;       // 0x0790
    float MaxSmoothedFrameRate;      // 0x0794
    float MinSmoothedFrameRate;      // 0x0798
    std::int32_t NumPawnsAllowedToBeSpawnedInAFrame; // 0x079C
    FPointer RemoteControlExec;      // 0x07A0
    FPointer MobileMaterialEmulator; // 0x07A8
    FColor C_WorldBox;               // 0x07B0
    FColor C_BrushWire;              // 0x07B4
    FColor C_AddWire;                // 0x07B8
    FColor C_SubtractWire;           // 0x07BC
    FColor C_SemiSolidWire;          // 0x07C0
    FColor C_NonSolidWire;           // 0x07C4
    FColor C_WireBackground;         // 0x07C8
    FColor C_ScaleBoxHi;             // 0x07CC
    FColor C_VolumeCollision;        // 0x07D0
    FColor C_BSPCollision;           // 0x07D4
    FColor C_OrthoBackground;        // 0x07D8
    FColor C_Volume;                 // 0x07DC
    FColor C_BrushShape;             // 0x07E0
    float StreamingDistanceFactor;   // 0x07E4
    FString ScoutClassName;          // 0x07E8
    std::uint8_t TransitionType;     // 0x07F8
    std::uint8_t Pad_07F9[0x07];     // 0x07F9
    FString TransitionDescription;   // 0x0800
    FString TransitionGameType;      // 0x0810
    float MeshLODRange;              // 0x0820
    float CameraRotationThreshold;   // 0x0824
    float CameraTranslationThreshold; // 0x0828
    float PrimitiveProbablyVisibleTime; // 0x082C
    float PercentUnoccludedRequeries; // 0x0830
    float MaxOcclusionPixelsFraction; // 0x0834
    std::int32_t MaxFluidNumVerts;   // 0x0838
    float FluidSimulationTimeLimit;  // 0x083C
    std::int32_t MaxParticleResize;  // 0x0840
    std::int32_t MaxParticleResizeWarn; // 0x0844
    std::int32_t MaxParticleVertexMemory; // 0x0848
    std::int32_t MaxParticleSpriteCount; // 0x084C
    std::int32_t MaxParticleSubUVCount; // 0x0850
    std::int32_t BeginUPTryCount;    // 0x0854
    TArray<std::uint8_t> PendingDroppedNotes; // 0x0858
    FString DynamicCoverMeshComponentName; // 0x0868
    float NetClientTicksPerSecond;   // 0x0878
    float MaxTrackedOcclusionIncrement; // 0x087C
    float TrackedOcclusionStepSize;  // 0x0880
    FLinearColor DefaultSelectedMaterialColor; // 0x0884
    FLinearColor DefaultHoveredMaterialColor; // 0x0894
    FLinearColor SelectedMaterialColor; // 0x08A4
    FLinearColor UnselectedMaterialColor; // 0x08B4
    std::uint32_t Pad_08C4;          // 0x08C4
    TArray<FString> IgnoreSimulatedFuncWarnings; // 0x08C8
    std::int32_t ScreenSaverInhibitorSemaphore; // 0x08D8
    std::uint32_t Pad_08DC;          // 0x08DC
    FPointer ScreenSaverInhibitor;   // 0x08E0
    UObject* GlobalTranslationContext; // 0x08E8
    FDouble LoadingMovieStartTime;   // 0x08F0
    FString MatineeCaptureName;      // 0x08F8
    FString MatineePackageCaptureName; // 0x0908
    FString VisibleLevelsForMatineeCapture; // 0x0918
    std::int32_t MatineeCaptureFPS;  // 0x0928
    std::int32_t MatineeCaptureType; // 0x092C
    float MapLoadTimePC;             // 0x0930
    float MapLoadTimePS4;            // 0x0934
    float MapLoadTimeXboxOne;        // 0x0938
    float MapLoadTimeSwitch;         // 0x093C
    FEngineDelegateStorage EventPreLaunchURL; // 0x0940, reflected as __EventPreLaunchURL__Delegate
};

class UGameViewportClient : public UScriptViewportClient {
public:
    [[nodiscard]] static UClass* StaticClass();

    FPointer VfTable_FExec;          // 0x0068
    FPointer Viewport;               // 0x0070
    FPointer ViewportFrame;          // 0x0078
    TArray<UObject*> GlobalInteractions; // 0x0080
    UClass* UIControllerClass;       // 0x0090
    UObject* UIController;           // 0x0098
    UObject* ViewportConsole;        // 0x00A0
    FExportShowFlagsMirror ShowFlags; // 0x00A8
    FString LoadingMessage;          // 0x00B8
    FString SavingMessage;           // 0x00C8
    FString ConnectingMessage;       // 0x00D8
    FString PausedMessage;           // 0x00E8
    FString PrecachingMessage;       // 0x00F8
    std::uint32_t ViewportClientFlags; // 0x0108, reflected bool bitfield
    FTitleSafeZoneArea TitleSafeZone; // 0x010C
    std::uint8_t GamepadInputAPI;    // 0x011C
    std::uint8_t DesiredSplitscreenType; // 0x011D
    std::uint8_t ActiveSplitscreenType; // 0x011E
    std::uint8_t Default2PSplitType; // 0x011F
    std::uint8_t Default3PSplitType; // 0x0120
    std::uint8_t Pad_0121[0x07];     // 0x0121
    TArray<std::int32_t> Gamepads;   // 0x0128
    TArray<std::uint8_t> SplitscreenInfo; // 0x0138
    FString ProgressMessage;         // 0x0148
    std::uint8_t Pad_0158[0x10];     // 0x0158
    float ProgressTimeOut;           // 0x0168
    float ProgressFadeTime;          // 0x016C
    TArray<FString> DebugProperties; // 0x0170
    FPointer ScaleformInteraction;   // 0x0180
    FEngineDelegateStorage HandleInputKey;  // 0x0188, reflected as __HandleInputKey__Delegate
    FEngineDelegateStorage HandleInputAxis; // 0x01A8, reflected as __HandleInputAxis__Delegate
    FEngineDelegateStorage HandleInputChar; // 0x01C8, reflected as __HandleInputChar__Delegate
    FEngineDelegateStorage EventScreenDraggedOnNewScreen; // 0x01E8, reflected as __EventScreenDraggedOnNewScreen__Delegate
    FEngineDelegateStorage EventScaleformEnabledChanged; // 0x0208, reflected as __EventScaleformEnabledChanged__Delegate
    FEngineDelegateStorage EventGamepadInputAPIChanged; // 0x0228, reflected as __EventGamepadInputAPIChanged__Delegate
    FEngineDelegateStorage EventGamepadConnectionStatusChanged; // 0x0248, reflected as __EventGamepadConnectionStatusChanged__Delegate
    FEngineDelegateStorage EventGampadConnected; // 0x0268, reflected as __EventGampadConnected__Delegate; RL spelling
    FEngineDelegateStorage EventGampadDisconnected; // 0x0288, reflected as __EventGampadDisconnected__Delegate; RL spelling

    [[nodiscard]] bool HasViewportClientFlag(std::uint32_t mask) const noexcept { return (ViewportClientFlags & mask) != 0; }
    [[nodiscard]] bool bShowTitleSafeZone() const noexcept { return HasViewportClientFlag(0x001u); }
    [[nodiscard]] bool bDisplayHardwareMouseCursor() const noexcept { return HasViewportClientFlag(0x002u); }
    [[nodiscard]] bool bOverrideDiffuseAndSpecular() const noexcept { return HasViewportClientFlag(0x004u); }
    [[nodiscard]] bool bIsPlayInEditorViewport() const noexcept { return HasViewportClientFlag(0x008u); }
    [[nodiscard]] bool bShowSystemMouseCursor() const noexcept { return HasViewportClientFlag(0x010u); }
    [[nodiscard]] bool bDisableWorldRendering() const noexcept { return HasViewportClientFlag(0x020u); }
    [[nodiscard]] bool bCapturedWorldRendering() const noexcept { return HasViewportClientFlag(0x040u); }
    [[nodiscard]] bool bDebugNoGFxUI() const noexcept { return HasViewportClientFlag(0x080u); }
};

class UGameEngine : public UEngine {
public:
    [[nodiscard]] static UClass* StaticClass();

    UObject* GPendingLevel;          // 0x0960
    FString PendingLevelPlayerControllerClassName; // 0x0968
    FURLMirror LastURL;              // 0x0978
    FURLMirror LastRemoteURL;        // 0x09D8
    TArray<FString> ServerActors;    // 0x0A38
    FString TravelURL;               // 0x0A48
    std::uint8_t TravelType;         // 0x0A58
    std::uint8_t Pad_0A59[0x03];     // 0x0A59
    std::uint32_t GameEngineFlags;   // 0x0A5C, reflected bool bitfield
    UObject* OnlineSubsystem;        // 0x0A60
    UObject* DLCEnumerator;          // 0x0A68
    FString DownloadableContentEnumeratorClassName; // 0x0A70
    UObject* DLCManager;             // 0x0A80
    FString DownloadableContentManagerClassName; // 0x0A88
    TArray<FString> LevelsToLoadForPendingMapChange; // 0x0A98
    TArray<UObject*> LoadedLevelsForPendingMapChange; // 0x0AA8
    FString PendingMapChangeFailureDescription; // 0x0AB8
    float MaxDeltaTime;              // 0x0AC8
    std::uint32_t Pad_0ACC;          // 0x0ACC
    FString SecondaryViewportClientClassName; // 0x0AD0
    TArray<UGameViewportClient*> SecondaryViewportClients; // 0x0AE0
    TArray<FPointer> SecondaryViewportFrames; // 0x0AF0
    TArray<std::uint8_t> PendingLevelStreamingStatusUpdates; // 0x0B00
    TArray<UObject*> ObjectReferencers; // 0x0B10
    TArray<std::uint8_t> PackagesToFullyLoad; // 0x0B20
    TArray<std::uint8_t> NamedNetDrivers; // 0x0B30
    TArray<FName> AnimTags;          // 0x0B40
    FEngineDelegateStorage EventTravelMapNotFound; // 0x0B50, reflected as __EventTravelMapNotFound__Delegate

    [[nodiscard]] bool HasGameEngineFlag(std::uint32_t mask) const noexcept { return (GameEngineFlags & mask) != 0; }
    [[nodiscard]] bool bWorldWasLoadedThisTick() const noexcept { return HasGameEngineFlag(0x001u); }
    [[nodiscard]] bool bCheckForMovieCapture() const noexcept { return HasGameEngineFlag(0x002u); }
    [[nodiscard]] bool bTriggerPostLoadMap() const noexcept { return HasGameEngineFlag(0x004u); }
    [[nodiscard]] bool bStartedLoadMapMovie() const noexcept { return HasGameEngineFlag(0x008u); }
    [[nodiscard]] bool bUnloadingMap() const noexcept { return HasGameEngineFlag(0x010u); }
    [[nodiscard]] bool bShouldCommitPendingMapChange() const noexcept { return HasGameEngineFlag(0x020u); }
    [[nodiscard]] bool bClearAnimSetLinkupCachesOnLoadMap() const noexcept { return HasGameEngineFlag(0x040u); }
    [[nodiscard]] bool bEnableSecondaryDisplay() const noexcept { return HasGameEngineFlag(0x080u); }
    [[nodiscard]] bool bEnableSecondaryViewport() const noexcept { return HasGameEngineFlag(0x100u); }
};

#if defined(__GNUC__)
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Winvalid-offsetof"
#endif

static_assert(sizeof(FURLMirror) == 0x60);
static_assert(offsetof(FURLMirror, Protocol) == 0x00);
static_assert(offsetof(FURLMirror, Host) == 0x10);
static_assert(offsetof(FURLMirror, Map) == 0x28);
static_assert(offsetof(FURLMirror, Op) == 0x38);
static_assert(offsetof(FURLMirror, Portal) == 0x48);
static_assert(offsetof(FURLMirror, Valid) == 0x58);
static_assert(sizeof(FShowFlagsMirror) == 0x10);
static_assert(offsetof(FShowFlagsMirror, Flags0) == 0x00);
static_assert(offsetof(FShowFlagsMirror, Flags1) == 0x08);
static_assert(sizeof(FExportShowFlagsMirror) == 0x10);
static_assert(sizeof(FTitleSafeZoneArea) == 0x10);
static_assert(sizeof(FLevelViewportInfo) == 0x1C);
static_assert(offsetof(FLevelViewportInfo, CamRotation) == 0x0C);
static_assert(offsetof(FLevelViewportInfo, CamOrthoZoom) == 0x18);
static_assert(sizeof(FCachedPhysSMData) == 0x10);
static_assert(sizeof(FKCachedConvexDataElement) == 0x10);
static_assert(sizeof(FKCachedConvexData) == 0x10);
static_assert(sizeof(FKCachedPerTriData) == 0x10);
static_assert(sizeof(FKSphereElem) == 0x48);
static_assert(sizeof(FKBoxElem) == 0x50);
static_assert(sizeof(FKSphylElem) == 0x50);
static_assert(sizeof(FKConvexElem) == 0x80);
static_assert(offsetof(FKConvexElem, ElemBox) == 0x60);
static_assert(sizeof(FKAggregateGeom) == 0x50);
static_assert(offsetof(FKAggregateGeom, RenderInfo) == 0x40);
static_assert(sizeof(FPrecomputedVisibilityCell) == 0x10);
static_assert(sizeof(FCompressedVisibilityChunk) == 0x18);
static_assert(sizeof(FPrecomputedVisibilityBucket) == 0x28);
static_assert(sizeof(FPrecomputedVisibilityHandler) == 0x30);
static_assert(offsetof(FPrecomputedVisibilityHandler, Id) == 0x18);
static_assert(offsetof(FPrecomputedVisibilityHandler, PrecomputedVisibilityCellBuckets) == 0x20);
static_assert(sizeof(FPrecomputedVolumeDistanceField) == 0x40);
static_assert(offsetof(FPrecomputedVolumeDistanceField, VolumeBox) == 0x04);
static_assert(offsetof(FPrecomputedVolumeDistanceField, Data) == 0x30);
static_assert(sizeof(FGuidPair) == 0x18);
static_assert(sizeof(FCoverIndexPair) == 0x08);
static_assert(sizeof(FRenderCommandFence) == 0x04);
static_assert(sizeof(FLevelNativeState) == 0x300);
static_assert(offsetof(FLevelNativeState, SceneIndex) == 0x000);
static_assert(offsetof(FLevelNativeState, CachedPhysBSPData) == 0x020);
static_assert(offsetof(FLevelNativeState, CachedPhysSMDataMap) == 0x038);
static_assert(offsetof(FLevelNativeState, CachedPhysSMDataStore) == 0x090);
static_assert(offsetof(FLevelNativeState, CachedPhysPerTriSMDataMap) == 0x0A0);
static_assert(offsetof(FLevelNativeState, ConvexBSPAggGeom) == 0x100);
static_assert(offsetof(FLevelNativeState, CachedPhysConvexBSPData) == 0x150);
static_assert(offsetof(FLevelNativeState, NavListStart) == 0x168);
static_assert(offsetof(FLevelNativeState, CrossLevelCoverGuidRefs) == 0x198);
static_assert(offsetof(FLevelNativeState, TickableActors) == 0x1C8);
static_assert(offsetof(FLevelNativeState, PrecomputedLightVolume) == 0x1F8);
static_assert(offsetof(FLevelNativeState, PrecomputedVisibilityHandler) == 0x200);
static_assert(offsetof(FLevelNativeState, PrecomputedVolumeDistanceField) == 0x230);
static_assert(offsetof(FLevelNativeState, RemoveFromSceneFence) == 0x270);
static_assert(offsetof(FLevelNativeState, AppliedLevelTransform) == 0x278);
static_assert(offsetof(FLevelNativeState, bAreComponentsCurrentlyAttached) == 0x2B8);
static_assert(offsetof(FLevelNativeState, CurrentActorIndexForInitActorsRBPhys) == 0x2F0);
static_assert(sizeof(FWorldGCReferencer) == 0x30);
static_assert(offsetof(FWorldGCReferencer, ViewportClient) == 0x10);
static_assert(offsetof(FWorldGCReferencer, PostProcessChain) == 0x28);
static_assert(sizeof(FWorldNativeTailState) == 0x48);
static_assert(offsetof(FWorldNativeTailState, UnusedNativePointer_0370) == 0x00);
static_assert(offsetof(FWorldNativeTailState, TutorialInstances) == 0x18);
static_assert(offsetof(FWorldNativeTailState, UnusedNativePointer_0398) == 0x28);
static_assert(offsetof(FWorldNativeTailState, bHasGCReferencers) == 0x30);
static_assert(offsetof(FWorldNativeTailState, GCReferencers) == 0x38);
static_assert(sizeof(USubsystem) == 0x68);
static_assert(sizeof(UScriptViewportClient) == 0x68);
static_assert(sizeof(ULevelBase) == 0xD8);
static_assert(offsetof(ULevelBase, Actors) == 0x60);
static_assert(offsetof(ULevelBase, URL) == 0x78);
static_assert(sizeof(ULevel) == 0x500);
static_assert(offsetof(ULevel, Model) == 0xD8);
static_assert(offsetof(ULevel, ModelComponents) == 0xE0);
static_assert(offsetof(ULevel, GameSequences) == 0xF0);
static_assert(offsetof(ULevel, TextureToInstancesMap) == 0x100);
static_assert(offsetof(ULevel, DynamicTextureInstances) == 0x150);
static_assert(offsetof(ULevel, ForceStreamTextures) == 0x1A0);
static_assert(offsetof(ULevel, iFirstNetRelevantActor) == 0x1F0);
static_assert(offsetof(ULevel, iFirstDynamicActor) == 0x1F4);
static_assert(offsetof(ULevel, LightmapTotalSize) == 0x1F8);
static_assert(offsetof(ULevel, ShadowmapTotalSize) == 0x1FC);
static_assert(offsetof(ULevel, NativeState) == 0x200);
static_assert(sizeof(UWorld) == 0x3B8);
static_assert(offsetof(UWorld, VfTable_FNetworkNotify) == 0x60);
static_assert(offsetof(UWorld, RLPreSceneLayoutSlot) == 0x68);
static_assert(offsetof(UWorld, Scene) == 0x70);
static_assert(offsetof(UWorld, Levels) == 0x78);
static_assert(offsetof(UWorld, PersistentLevel) == 0x88);
static_assert(offsetof(UWorld, CurrentLevel) == 0x98);
static_assert(offsetof(UWorld, EditorViews) == 0xB0);
static_assert(offsetof(UWorld, SaveGameSummary_DEPRECATED) == 0x120);
static_assert(offsetof(UWorld, NetDriver) == 0x128);
static_assert(offsetof(UWorld, URL) == 0x130);
static_assert(offsetof(UWorld, DemoRecDriver) == 0x190);
static_assert(offsetof(UWorld, Hash) == 0x1A0);
static_assert(offsetof(UWorld, NewlySpawned) == 0x1C8);
static_assert(offsetof(UWorld, TickGroup) == 0x1E0);
static_assert(offsetof(UWorld, RBPhysScene) == 0x210);
static_assert(offsetof(UWorld, BodyInstancePool) == 0x220);
static_assert(offsetof(UWorld, TimeSinceLastPendingKillPurge) == 0x250);
static_assert(offsetof(UWorld, StaticLightList) == 0x260);
static_assert(offsetof(UWorld, DynamicLightList) == 0x288);
static_assert(offsetof(UWorld, LightEnvironmentList) == 0x2B0);
static_assert(offsetof(UWorld, DominantDirectionalLight) == 0x2D8);
static_assert(offsetof(UWorld, DominantSpotLights) == 0x2E0);
static_assert(offsetof(UWorld, DominantPointLights) == 0x308);
static_assert(offsetof(UWorld, ExtraReferencedObjects) == 0x340);
static_assert(offsetof(UWorld, Observers) == 0x360);
static_assert(offsetof(UWorld, NativeTailState) == 0x370);
static_assert(sizeof(UEngine) == 0x960);
static_assert(offsetof(UEngine, GameShareClassName) == 0x68);
static_assert(offsetof(UEngine, BuildID) == 0x98);
static_assert(offsetof(UEngine, TinyFont) == 0xA8);
static_assert(offsetof(UEngine, SubtitleFont) == 0x108);
static_assert(offsetof(UEngine, AdditionalFonts) == 0x138);
static_assert(offsetof(UEngine, DefaultTexture) == 0x1E8);
static_assert(offsetof(UEngine, BuildingQuadStaticMesh) == 0x428);
static_assert(offsetof(UEngine, MaxRMSDForCombiningMappings) == 0x458);
static_assert(offsetof(UEngine, LightingOnlyBrightness) == 0x460);
static_assert(offsetof(UEngine, StatColorMappings) == 0x4D8);
static_assert(offsetof(UEngine, OnlineSubsystemClass) == 0x570);
static_assert(offsetof(UEngine, GameViewport) == 0x770);
static_assert(offsetof(UEngine, C_WorldBox) == 0x7B0);
static_assert(offsetof(UEngine, TransitionDescription) == 0x800);
static_assert(offsetof(UEngine, DefaultSelectedMaterialColor) == 0x884);
static_assert(offsetof(UEngine, LoadingMovieStartTime) == 0x8F0);
static_assert(offsetof(UEngine, EventPreLaunchURL) == 0x940);
static_assert(sizeof(UGameViewportClient) == 0x2A8);
static_assert(offsetof(UGameViewportClient, VfTable_FExec) == 0x68);
static_assert(offsetof(UGameViewportClient, GlobalInteractions) == 0x80);
static_assert(offsetof(UGameViewportClient, ViewportClientFlags) == 0x108);
static_assert(offsetof(UGameViewportClient, HandleInputKey) == 0x188);
static_assert(offsetof(UGameViewportClient, EventGampadDisconnected) == 0x288);
static_assert(sizeof(UGameEngine) == 0xB70);
static_assert(offsetof(UGameEngine, GPendingLevel) == 0x960);
static_assert(offsetof(UGameEngine, LastURL) == 0x978);
static_assert(offsetof(UGameEngine, LastRemoteURL) == 0x9D8);
static_assert(offsetof(UGameEngine, ServerActors) == 0xA38);
static_assert(offsetof(UGameEngine, GameEngineFlags) == 0xA5C);
static_assert(offsetof(UGameEngine, EventTravelMapNotFound) == 0xB50);

#if defined(__GNUC__)
#pragma GCC diagnostic pop
#endif

} // namespace Kuru::SDK
