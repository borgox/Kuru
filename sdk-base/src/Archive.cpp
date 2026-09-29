#include <Core/Archive.hpp>

#include <Core/Names.hpp>
#include <Core/ObjectModel.hpp>
#include <Core/Strings.hpp>

namespace Kuru::SDK {

void FArchive::SerializeInt(std::uint32_t& value, std::uint32_t max) {
    (void)max;
    Serialize(&value, sizeof(value));
}

FArchive& operator<<(FArchive& archive, FString& value) {
    std::int32_t count = value.Count;
    archive << count;
    if (archive.IsLoading()) {
        value.SetNumZeroed(count);
    }
    if (count > 0 && value.Data) {
        archive.Serialize(value.Data, static_cast<std::size_t>(count) * sizeof(wchar_t));
    }
    return archive;
}

FArchive& operator<<(FArchive& archive, FName& value) {
    archive << value.Index;
    archive << value.Number;
    return archive;
}

FArchive& operator<<(FArchive& archive, UObject*& value) {
    archive.Serialize(&value, sizeof(value));
    return archive;
}

} // namespace Kuru::SDK
