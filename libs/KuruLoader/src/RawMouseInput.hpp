#pragma once
namespace Kuru::RawMouseInput {
bool Install();
void Capture(bool mouse);
bool Disable();
bool WaitForIdle();
void Shutdown();
}
