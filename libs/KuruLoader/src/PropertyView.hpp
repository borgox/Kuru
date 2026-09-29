#pragma once

#include <cstddef>

namespace Kuru::SDK {
class UProperty;
}


namespace Kuru::PropertyView {

const char* KindOf(Kuru::SDK::UProperty* prop);
bool Draw(Kuru::SDK::UProperty* prop, unsigned char* base, std::size_t size);

} // namespace Kuru::PropertyView
