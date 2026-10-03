#pragma once

#include <Kuru/Api.hpp>

#include <cstdint>
#include <vector>

/* Kuru::Game convenience finders that hand you the typed SDK objects.
 * They walk the live object graph from a cached engine root (O(1) pointer reads) instead of
 * scanning GObjects. Call them on the GAME thread.
 */
namespace Kuru::Game {

// The one game engine, found once and cached (it lives the whole session). Engine.Engine.GetEngine is a native static function that simply returns GEngine, so we call its Func pointer directly
inline Kuru::SDK::UEngine* GetEngine() {
    static Kuru::SDK::UEngine* cached = nullptr;
    if (cached) {
        return cached;
    }
    Kuru::SDK::UFunction* fn = Kuru::SDK::UObject::FindFunction("Engine.Engine.GetEngine");
    if (fn && fn->Func) {
        std::uint8_t endParms[8] = {};  // no input params, the native just finalizes and returns GEngine
        Kuru::SDK::FFrame frame{};
        frame.Node = reinterpret_cast<Kuru::SDK::UStruct*>(fn);
        frame.Object = fn;  // context (ignored by a static getter, but matches the call)
        frame.Code = endParms;
        Kuru::SDK::UEngine* result = nullptr;
        using GetEngineExec = void(__fastcall*)(Kuru::SDK::UObject*, Kuru::SDK::FFrame&, void*);
        reinterpret_cast<GetEngineExec>(fn->Func)(fn, frame, &result);
        cached = result;
    }
    return cached;
}

// Is this object still live (present in GObjects)? O(n): for validating a cached pointer.
inline bool IsValid(Kuru::SDK::UObject* object) {
    if (!object) {
        return false;
    }
    if (auto* objects = Kuru::SDK::GetGlobalObjects()) {
        for (Kuru::SDK::UObject* candidate : *objects) {
            if (candidate == object) {
                return true;
            }
        }
    }
    return false;
}

// --- Local player ---

inline Kuru::RL::ULocalPlayer_TA* GetLocalPlayer(int index = 0) {
    Kuru::SDK::UEngine* engine = GetEngine();
    if (!engine || index < 0 || index >= engine->GamePlayers.Num()) {
        return nullptr;
    }
    return reinterpret_cast<Kuru::RL::ULocalPlayer_TA*>(engine->GamePlayers[index]);
}

inline Kuru::RL::APlayerController_TA* GetPlayerController(int index = 0) {
    Kuru::RL::ULocalPlayer_TA* player = GetLocalPlayer(index);
    return player ? reinterpret_cast<Kuru::RL::APlayerController_TA*>(player->Actor) : nullptr;
}

inline Kuru::RL::ACar_TA* GetLocalCar() {
    Kuru::RL::APlayerController_TA* pc = GetPlayerController();
    return pc ? pc->Car : nullptr;
}

inline Kuru::RL::APawn* GetLocalPawn() {
    Kuru::RL::APlayerController_TA* pc = GetPlayerController();
    return pc ? pc->Pawn : nullptr;
}

inline Kuru::RL::AHUD* GetHUD() {
    Kuru::RL::APlayerController_TA* pc = GetPlayerController();
    return pc ? pc->myHUD : nullptr;
}

inline Kuru::RL::ACamera* GetCamera() {
    Kuru::RL::APlayerController_TA* pc = GetPlayerController();
    return pc ? pc->PlayerCamera : nullptr;
}

inline Kuru::RL::APRI_TA* GetLocalPRI() {
    Kuru::RL::APlayerController_TA* pc = GetPlayerController();
    return pc ? reinterpret_cast<Kuru::RL::APRI_TA*>(pc->PlayerReplicationInfo) : nullptr;
}

// --- Match / game event ---

inline Kuru::RL::AGameEvent_TA* GetGameEvent() {
    Kuru::SDK::UEngine* engine = GetEngine();
    if (!engine || !engine->GameViewport) {
        return nullptr;
    }
    auto* viewport = reinterpret_cast<Kuru::RL::UGameViewportClient_TA*>(engine->GameViewport);
    return viewport->GameEvent;  // UGameViewportClient_TA::GameEvent @ 0x568
}

inline Kuru::RL::AGameEvent_Soccar_TA* GetSoccarGameEvent() {
    Kuru::RL::AGameEvent_TA* event = GetGameEvent();
    if (event && event->IsA(Kuru::RL::AGameEvent_Soccar_TA::StaticClass())) {
        return reinterpret_cast<Kuru::RL::AGameEvent_Soccar_TA*>(event);
    }
    return nullptr;
}

inline bool IsInGame() {
    return GetGameEvent() != nullptr;
}

inline int GetSecondsRemaining() {
    Kuru::RL::AGameEvent_Soccar_TA* soccar = GetSoccarGameEvent();
    return soccar ? soccar->SecondsRemaining : -1;
}

// --- Collections  ---

inline std::vector<Kuru::RL::ACar_TA*> GetCars() {
    std::vector<Kuru::RL::ACar_TA*> out;
    if (Kuru::RL::AGameEvent_TA* event = GetGameEvent()) {
        auto& arr = *reinterpret_cast<Kuru::SDK::TArray<Kuru::RL::ACar_TA*>*>(&event->Cars);
        for (int i = 0; i < arr.Num(); ++i) {
            if (arr[i]) out.push_back(arr[i]);
        }
    }
    return out;
}

inline std::vector<Kuru::RL::APRI_TA*> GetPRIs() {
    std::vector<Kuru::RL::APRI_TA*> out;
    if (Kuru::RL::AGameEvent_TA* event = GetGameEvent()) {
        auto& arr = *reinterpret_cast<Kuru::SDK::TArray<Kuru::RL::APRI_TA*>*>(&event->PRIs);
        for (int i = 0; i < arr.Num(); ++i) {
            if (arr[i]) out.push_back(arr[i]);
        }
    }
    return out;
}

// --- Generic power-tools O(n) use rarely ---

template <typename T>
inline std::vector<T*> GetAllOfType() {
    std::vector<T*> out;
    Kuru::SDK::UClass* cls = T::StaticClass();
    if (auto* objects = Kuru::SDK::GetGlobalObjects()) {
        for (Kuru::SDK::UObject* object : *objects) {
            if (object && cls && object->IsA(cls) && !object->IsTemplate()) {
                out.push_back(reinterpret_cast<T*>(object));
            }
        }
    }
    return out;
}

template <typename T>
inline T* GetFirstOfType() {
    Kuru::SDK::UClass* cls = T::StaticClass();
    if (auto* objects = Kuru::SDK::GetGlobalObjects()) {
        for (Kuru::SDK::UObject* object : *objects) {
            if (object && cls && object->IsA(cls) && !object->IsTemplate()) {
                return reinterpret_cast<T*>(object);
            }
        }
    }
    return nullptr;
}

// Primary ball via the soccar game event, falls back to the first live ball.
inline Kuru::RL::ABall_TA* GetBall() {
    if (Kuru::RL::AGameEvent_Soccar_TA* soccar = GetSoccarGameEvent()) {
        auto& balls = *reinterpret_cast<Kuru::SDK::TArray<Kuru::RL::ABall_TA*>*>(&soccar->GameBalls);
        if (balls.Num() > 0 && balls[0]) {
            return balls[0];
        }
    }
    return GetFirstOfType<Kuru::RL::ABall_TA>();
}

} // namespace Kuru::Game
