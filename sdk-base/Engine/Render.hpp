#pragma once

#include <Core/Containers.hpp>
#include <Core/Math.hpp>
#include <Core/Misc.hpp>
#include <Core/Names.hpp>
#include <Core/ObjectModel.hpp>
#include <Core/PropertyTag.hpp>
#include <Core/Strings.hpp>

#include <cstdint>

namespace Kuru::SDK {

class UTexture;
class UTexture2D;
class UTextureRenderTarget2D;

struct FUntypedBulkDataMirror {
    FPointer VfTable;                // 0x0000
    std::int32_t BulkDataFlags;      // 0x0008
    std::int32_t ElementCount;       // 0x000C
    std::uint64_t BulkDataOffsetInFile; // 0x0010
    std::int32_t BulkDataSizeOnDisk; // 0x0018
    std::int32_t SavedBulkDataFlags; // 0x001C
    std::int32_t SavedElementCount;  // 0x0020
    // 0x0024: C++ padding to 8-align the next field (compiler-inserted)
    std::uint64_t SavedBulkDataOffsetInFile; // 0x0028
    std::int32_t SavedBulkDataSizeOnDisk; // 0x0030
    // 0x0034: C++ padding
    FPointer BulkData;               // 0x0038
    std::int32_t LockStatus;         // 0x0040
    // 0x0044: C++ padding
    FPointer AttachedAr;             // 0x0048
    std::int32_t bShouldFreeOnEmpty; // 0x0050
    // 0x0054: trailing C++ padding (struct rounds up to 0x58)
};

struct FIndirectArrayMirror {
    void* Data = nullptr;
    std::int32_t Count = 0;
    std::int32_t Max = 0;
};

struct FThreadSafeCounter {
    std::int32_t Value = 0;
};

struct FTextureLinkedListMirror {
    void* Previous = nullptr;
    void* Next = nullptr;
    UTexture2D* Texture = nullptr;
};

class FRenderResource {
public:
    virtual ~FRenderResource() = default;
    virtual void InitDynamicRHI() {}
    virtual void ReleaseDynamicRHI() {}
    virtual void InitRHI() {}
    virtual void ReleaseRHI() {}
};

class FTexture : public FRenderResource {
public:
    std::uint32_t SizeX = 0;
    std::uint32_t SizeY = 0;
};

class FD3D11Texture2D : public FTexture {
public:
    void* Resource = nullptr;
    void* ShaderResourceView = nullptr;
    void* RenderTargetView = nullptr;
    void* DepthStencilView = nullptr;
};

class USurface : public UObject {
public:
    [[nodiscard]] static UClass* StaticClass();
};

class UTexture : public USurface {
public:
    std::uint32_t TextureFlags;      // 0x0060, reflected bool bitfield
    float UnpackMin[4];              // 0x0064
    float UnpackMax[4];              // 0x0074
    std::uint32_t Pad_0084;          // 0x0084
    FUntypedBulkDataMirror SourceArt; // 0x0088
    std::uint8_t CompressionSettings; // 0x00E0
    std::uint8_t Filter;             // 0x00E1
    std::uint8_t LODGroup;           // 0x00E2
    std::uint8_t CachedLODGroup;     // 0x00E3
    std::uint8_t MipGenSettings;     // 0x00E4
    std::uint8_t Pad_00E5[0x03];     // 0x00E5
    std::int32_t LODBias;            // 0x00E8
    std::int32_t CachedCombinedLODBias; // 0x00EC
    std::int32_t NumCinematicMipLevels; // 0x00F0
    std::uint32_t Pad_00F4;          // 0x00F4
    FString SourceFilePath;          // 0x00F8
    FString SourceFileTimestamp;     // 0x0108
    FPointer Resource;               // 0x0118
    FGuid LightingGuid;              // 0x0120
    float AdjustBrightness;          // 0x0130
    float AdjustBrightnessCurve;     // 0x0134
    float AdjustVibrance;            // 0x0138
    float AdjustSaturation;          // 0x013C
    float AdjustRGBCurve;            // 0x0140
    float AdjustHue;                 // 0x0144
    std::int32_t InternalFormatLODBias; // 0x0148
    std::int32_t RequiredCreationFlags; // 0x014C

    [[nodiscard]] static UClass* StaticClass();
    [[nodiscard]] bool HasTextureFlag(std::uint32_t mask) const noexcept { return (TextureFlags & mask) != 0; }
    [[nodiscard]] bool SRGB() const noexcept { return HasTextureFlag(0x00001u); }
    [[nodiscard]] bool RGBE() const noexcept { return HasTextureFlag(0x00002u); }
    [[nodiscard]] bool bIsSourceArtUncompressed() const noexcept { return HasTextureFlag(0x00004u); }
    [[nodiscard]] bool CompressionNoAlpha() const noexcept { return HasTextureFlag(0x00008u); }
    [[nodiscard]] bool CompressionNone() const noexcept { return HasTextureFlag(0x00010u); }
    [[nodiscard]] bool CompressionNoMipmaps() const noexcept { return HasTextureFlag(0x00020u); }
    [[nodiscard]] bool CompressionFullDynamicRange() const noexcept { return HasTextureFlag(0x00040u); }
    [[nodiscard]] bool DeferCompression() const noexcept { return HasTextureFlag(0x00080u); }
    [[nodiscard]] bool NeverStream() const noexcept { return HasTextureFlag(0x00100u); }
    [[nodiscard]] bool bDitherMipMapAlpha() const noexcept { return HasTextureFlag(0x00200u); }
    [[nodiscard]] bool bPreserveBorderR() const noexcept { return HasTextureFlag(0x00400u); }
    [[nodiscard]] bool bPreserveBorderG() const noexcept { return HasTextureFlag(0x00800u); }
    [[nodiscard]] bool bPreserveBorderB() const noexcept { return HasTextureFlag(0x01000u); }
    [[nodiscard]] bool bPreserveBorderA() const noexcept { return HasTextureFlag(0x02000u); }
    [[nodiscard]] bool bNoTiling() const noexcept { return HasTextureFlag(0x04000u); }
    [[nodiscard]] bool bForcePVRTC4() const noexcept { return HasTextureFlag(0x08000u); }
    [[nodiscard]] bool bAsyncResourceReleaseHasBeenStarted() const noexcept { return HasTextureFlag(0x10000u); }
    [[nodiscard]] bool bUseCinematicMipLevels() const noexcept { return HasTextureFlag(0x20000u); }
};

class UTexture2D : public UTexture {
public:
    FIndirectArrayMirror Mips;       // 0x0150
    FIndirectArrayMirror CachedPVRTCMips; // 0x0160
    FIndirectArrayMirror CachedATITCMips; // 0x0170
    FIndirectArrayMirror CachedETCMips; // 0x0180
    std::int32_t CachedFlashMipsMaxResolution; // 0x0190
    std::uint32_t Pad_0194;          // 0x0194
    FUntypedBulkDataMirror CachedFlashMips; // 0x0198
    std::int32_t SizeX;              // 0x01F0
    std::int32_t SizeY;              // 0x01F4
    std::int32_t OriginalSizeX;      // 0x01F8
    std::int32_t OriginalSizeY;      // 0x01FC
    std::uint8_t Format;             // 0x0200
    std::uint8_t AddressX;           // 0x0201
    std::uint8_t AddressY;           // 0x0202
    std::uint8_t Pad_0203;           // 0x0203
    std::uint32_t Texture2DFlags;    // 0x0204, reflected bool bitfield
    float ForceMipLevelsToBeResidentTimestamp; // 0x0208
    FName TextureFileCacheName;      // 0x020C
    FGuid TextureFileCacheGuid;      // 0x0214
    std::int32_t RequestedMips;      // 0x0224
    std::int32_t ResidentMips;       // 0x0228
    std::int32_t MipsToRemoveOnCompress; // 0x022C
    FThreadSafeCounter PendingMipChangeRequestStatus; // 0x0230
    std::uint32_t Pad_0234;          // 0x0234
    TArray<std::uint8_t> SystemMemoryData; // 0x0238
    UTextureRenderTarget2D* RenderTarget2DRef; // 0x0248
    FTextureLinkedListMirror StreamableTexturesLink; // 0x0250
    std::int32_t StreamingIndex;     // 0x0268
    std::int32_t MipTailBaseIdx;     // 0x026C
    FPointer ResourceMem;            // 0x0270
    std::int32_t FirstResourceMemMip; // 0x0278
    float Timer;                     // 0x027C

    [[nodiscard]] static UClass* StaticClass();
    [[nodiscard]] bool HasTexture2DFlag(std::uint32_t mask) const noexcept { return (Texture2DFlags & mask) != 0; }
    [[nodiscard]] bool bIsEditorOnly() const noexcept { return HasTexture2DFlag(0x001u); }
    [[nodiscard]] bool bIsStreamable() const noexcept { return HasTexture2DFlag(0x002u); }
    [[nodiscard]] bool bHasCancelationPending() const noexcept { return HasTexture2DFlag(0x004u); }
    [[nodiscard]] bool bHasBeenLoadedFromPersistentArchive() const noexcept { return HasTexture2DFlag(0x008u); }
    [[nodiscard]] bool bForceMiplevelsToBeResident() const noexcept { return HasTexture2DFlag(0x010u); }
    [[nodiscard]] bool bGlobalForceMipLevelsToBeResident() const noexcept { return HasTexture2DFlag(0x020u); }
    [[nodiscard]] bool bIsCompositingSource() const noexcept { return HasTexture2DFlag(0x040u); }
    [[nodiscard]] bool bHasBeenPaintedInEditor() const noexcept { return HasTexture2DFlag(0x080u); }
    [[nodiscard]] bool bUseAlphaInThumbnail() const noexcept { return HasTexture2DFlag(0x100u); }
    [[nodiscard]] std::int32_t GetSizeX() const noexcept { return SizeX; }
    [[nodiscard]] std::int32_t GetSizeY() const noexcept { return SizeY; }
    [[nodiscard]] std::uint8_t GetPixelFormat() const noexcept { return Format; }
    [[nodiscard]] FPointer GetResource() const noexcept { return Resource; }
    [[nodiscard]] bool IsStreamable() const noexcept { return StreamingIndex >= 0; }
    [[nodiscard]] static UTexture2D* GetDefaultObject() { return Kuru::SDK::GetDefaultObject<UTexture2D>(); }
};

#if defined(__GNUC__)
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Winvalid-offsetof"
#endif

static_assert(sizeof(FUntypedBulkDataMirror) == 0x58);
static_assert(offsetof(FUntypedBulkDataMirror, BulkDataFlags) == 0x08);
static_assert(offsetof(FUntypedBulkDataMirror, BulkDataOffsetInFile) == 0x10);
static_assert(offsetof(FUntypedBulkDataMirror, SavedBulkDataOffsetInFile) == 0x28);
static_assert(offsetof(FUntypedBulkDataMirror, BulkData) == 0x38);
static_assert(offsetof(FUntypedBulkDataMirror, AttachedAr) == 0x48);
static_assert(sizeof(FIndirectArrayMirror) == 0x10);
static_assert(sizeof(FThreadSafeCounter) == 0x04);
static_assert(sizeof(FTextureLinkedListMirror) == 0x18);
static_assert(sizeof(USurface) == 0x60);
static_assert(sizeof(UTexture) == 0x150);
static_assert(offsetof(UTexture, TextureFlags) == 0x60);
static_assert(offsetof(UTexture, SourceArt) == 0x88);
static_assert(offsetof(UTexture, CompressionSettings) == 0xE0);
static_assert(offsetof(UTexture, Resource) == 0x118);
static_assert(offsetof(UTexture, LightingGuid) == 0x120);
static_assert(sizeof(UTexture2D) == 0x280);
static_assert(offsetof(UTexture2D, Mips) == 0x150);
static_assert(offsetof(UTexture2D, CachedFlashMips) == 0x198);
static_assert(offsetof(UTexture2D, SizeX) == 0x1F0);
static_assert(offsetof(UTexture2D, Texture2DFlags) == 0x204);
static_assert(offsetof(UTexture2D, TextureFileCacheName) == 0x20C);
static_assert(offsetof(UTexture2D, SystemMemoryData) == 0x238);
static_assert(offsetof(UTexture2D, StreamableTexturesLink) == 0x250);

#if defined(__GNUC__)
#pragma GCC diagnostic pop
#endif

} // namespace Kuru::SDK
