#include <Kuru/Kuru.hpp>

namespace {

class MyMod final : public Kuru::IMod {
public:
    bool OnLoad() override {
        Kuru::Info("MyMod loaded");

        Kuru::Commands::Register("mymod", "mymod - example command", [](const char*) {
            Kuru::Info("hello from MyMod");
        });

        // Ball_TA.OnHitGoal fires when a goal is scored.
        Kuru::Hooks::Post("Function TAGame.Ball_TA.OnHitGoal", [](Kuru::HookContext&) {
            Kuru::Info("goal scored");
        });
        return true;
    }
};

}  // namespace

KURU_EXPORT_MOD(MyMod, "MyMod", "your name", "0.1.0")
