#include <Engine/Render.hpp>

namespace Kuru::SDK {

UClass* USurface::StaticClass() { return UObject::FindClass("Engine.Surface"); }
UClass* UTexture::StaticClass() { return UObject::FindClass("Engine.Texture"); }
UClass* UTexture2D::StaticClass() { return UObject::FindClass("Engine.Texture2D"); }

} // namespace Kuru::SDK
