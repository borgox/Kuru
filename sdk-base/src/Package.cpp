#include <Core/Package.hpp>

namespace Kuru::SDK {

UClass* ULinkerLoad::StaticClass() { return UObject::FindClass("Core.LinkerLoad"); }
UClass* ULinkerSave::StaticClass() { return UObject::FindClass("Core.LinkerSave"); }
UClass* UPackage::StaticClass() { return UObject::FindClass("Core.Package"); }
UClass* UPackageMap::StaticClass() { return UObject::FindClass("Core.PackageMap"); }

ULinkerLoad* ULinkerLoad::CreateLinker(UPackage* parent, const wchar_t* filename, std::uint32_t loadFlags) {
    (void)parent;
    (void)filename;
    (void)loadFlags;
    return nullptr;
}

void ULinkerLoad::Detach() {}

void ULinkerLoad::Preload(UObject* object) {
    (void)object;
}

void ULinkerLoad::Serialize(void* data, std::int32_t length) {
    (void)data;
    (void)length;
}

void ULinkerLoad::Seek(std::int32_t position) {
    NativeLoadState.ArchiveState.ArMaxSerializeSize = position;
}

std::int32_t ULinkerLoad::Tell() const {
    return NativeLoadState.ArchiveState.ArMaxSerializeSize;
}

std::int32_t ULinkerLoad::TotalSize() const {
    return Summary.ExportOffset > 0 ? Summary.ExportOffset : 0;
}

void ULinkerSave::Detach() {}

void ULinkerSave::Serialize(void* data, std::int32_t length) {
    (void)data;
    (void)length;
}

void ULinkerSave::Seek(std::int32_t position) {
    NativeSaveState.ArchiveState.ArMaxSerializeSize = position;
}

std::int32_t ULinkerSave::Tell() const {
    return NativeSaveState.ArchiveState.ArMaxSerializeSize;
}

} // namespace Kuru::SDK
