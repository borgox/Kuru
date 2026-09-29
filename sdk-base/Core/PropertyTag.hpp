#pragma once

#include "Names.hpp"

#include <cstdint>
#include <string>

namespace Kuru::SDK {

class FArchive;

/* Runtime/package property tag metadata. Exact serialized variants can differ by
  licensee version, so this is intentionally a foundation declaration rather than
  a full linker clone.
*/
struct FPropertyTag {
    FName Name;
    FName Type;
    std::int32_t Size = 0;
    std::int32_t ArrayIndex = 0;
    FName StructName;
    FName EnumName;
    FName InnerName;
    std::uint8_t BoolValue = 0;

    [[nodiscard]] bool IsNone() const noexcept { return Name.IsNone(); }
    void Serialize(FArchive& archive);
};

struct FCompressedChunk {
    std::int32_t UncompressedOffset = 0;
    std::int32_t UncompressedSize = 0;
    std::int32_t CompressedOffset = 0;
    std::int32_t CompressedSize = 0;

    [[nodiscard]] bool IsValid() const noexcept {
        return UncompressedSize > 0 && CompressedSize > 0;
    }
};

struct FGuid {
    std::uint32_t A = 0;
    std::uint32_t B = 0;
    std::uint32_t C = 0;
    std::uint32_t D = 0;

    [[nodiscard]] bool IsValid() const noexcept { return A != 0 || B != 0 || C != 0 || D != 0; }
    [[nodiscard]] std::string ToString() const;
    static bool Parse(const std::string& text, FGuid& outGuid);
};

static_assert(sizeof(FGuid) == 0x10);

} // namespace Kuru::SDK
