#pragma once
#include <cstdint>
namespace Kuru::RawMouseInput {
// Raw button down/up bits are paired: left, right, middle, button 4, button 5.
constexpr std::uint16_t SuppressButtons(std::uint16_t flags, bool capture, std::uint16_t& owned) {
    std::uint16_t remove = 0;
    for (unsigned i = 0; i < 5; ++i) {
        const auto down = std::uint16_t(1u << (i * 2));
        const auto up = std::uint16_t(down << 1);
        if ((flags & down) && capture) owned |= down;
        if (owned & down) {
            remove |= down | up;
            if (flags & up) owned &= ~down;
        }
    }
    return remove;
}
constexpr bool CheckGestureOwnership() {
    std::uint16_t owned = 0;
    if ((SuppressButtons(1, true, owned) & 1) == 0 || owned != 1) return false;
    if ((SuppressButtons(2, false, owned) & 2) == 0 || owned != 0) return false;
    if (SuppressButtons(2, true, owned) != 0) return false; // game-owned release must pass
    if (SuppressButtons(1, false, owned) != 0) return false;
    if (SuppressButtons(2, true, owned) != 0) return false;
    if ((SuppressButtons(0x101, true, owned) & 0x101) != 0x101) return false;
    if ((SuppressButtons(0x202, false, owned) & 0x202) != 0x202 || owned != 0) return false;
    return true;
}
static_assert(CheckGestureOwnership());
}
