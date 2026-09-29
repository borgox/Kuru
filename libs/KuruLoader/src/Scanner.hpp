#pragma once

namespace Kuru::SDK {
class UObject;
class UFunction;
}

// Function Scanner / call inspector

namespace Kuru::Scanner {

[[nodiscard]] bool IsEnabled();
void SetEnabled(bool enabled);
void Record(Kuru::SDK::UObject* caller, Kuru::SDK::UFunction* function, void* params);
void Draw();  // the "Functions" tab UI

} // namespace Kuru::Scanner
