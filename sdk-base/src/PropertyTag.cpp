#include <Core/PropertyTag.hpp>

#include <Core/Archive.hpp>

#include <cstdio>
#include <cstdlib>

namespace Kuru::SDK {

void FPropertyTag::Serialize(FArchive& archive) {
    archive << Name;
    if (Name.IsNone()) {
        return;
    }
    archive << Type;
    archive << Size;
    archive << ArrayIndex;
    archive << StructName;
    archive << EnumName;
    archive << InnerName;
    archive.Serialize(&BoolValue, sizeof(BoolValue));
}

std::string FGuid::ToString() const {
    char buffer[64]{};
    std::snprintf(buffer, sizeof(buffer), "%08X-%08X-%08X-%08X", A, B, C, D);
    return buffer;
}

bool FGuid::Parse(const std::string& text, FGuid& outGuid) {
    unsigned int a = 0;
    unsigned int b = 0;
    unsigned int c = 0;
    unsigned int d = 0;
    if (std::sscanf(text.c_str(), "%8x-%8x-%8x-%8x", &a, &b, &c, &d) != 4) {
        return false;
    }
    outGuid.A = a;
    outGuid.B = b;
    outGuid.C = c;
    outGuid.D = d;
    return true;
}

} // namespace Kuru::SDK
