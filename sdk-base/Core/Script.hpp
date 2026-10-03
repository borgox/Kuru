#pragma once

#include "Archive.hpp"
#include "ObjectModel.hpp"
#include "PropertyTag.hpp"

#include <cstdint>

namespace Kuru::SDK {

struct FOutParmRec {
    UProperty* Property = nullptr;
    std::uint8_t* PropAddr = nullptr;
    FOutParmRec* NextOutParm = nullptr;
};

struct FOutputDevice {
    void* VfTable = nullptr;             // 0x0000
    std::uint8_t Body[0x10]{};           // 0x0008, FOutputDevice body (flags + RL fork tail)
};

struct FFrame : FOutputDevice {
    UStruct* Node = nullptr;             // 0x0018, IDA-confirmed (ProcessInternal reads [frame+0x18])
    UObject* Object = nullptr;           // 0x0020, IDA-confirmed
    std::uint8_t* Code = nullptr;        // 0x0028, IDA-confirmed
    std::uint8_t* Locals = nullptr;      // 0x0030, IDA-confirmed (matches the raw offset our PI detour uses)
    FFrame* PreviousFrame = nullptr;     // 0x0038, inferred (+0x10 vs stock); NOT IDA-verified
    FOutParmRec* OutParms = nullptr;     // 0x0040, inferred (+0x10 vs stock); NOT IDA-verified

    FFrame() = default;
    FFrame(UObject* object, UStruct* node, std::uint8_t* code, std::uint8_t* locals);

    void Serialize(FArchive& archive);
    [[nodiscard]] std::int32_t ReadInt();
    [[nodiscard]] float ReadFloat();
    [[nodiscard]] FName ReadName();
    [[nodiscard]] UObject* ReadObject();
    [[nodiscard]] std::uint16_t ReadWord();
    [[nodiscard]] std::uint16_t ReadCodeSkipCount();
    [[nodiscard]] std::int32_t ReadVariableSize();
    [[nodiscard]] FString GetStackTrace() const;
};

struct FPushedState {
    UState* State = nullptr;
    UStruct* Node = nullptr;
    std::uint8_t* Code = nullptr;
};

// FStateFrame embeds FFrame (0x48 in the fork). These tail offsets are inferred from the FFrame +0x10 shift, NOT IDA-verified, FStateFrame is not load-bearing (nothing reads it in a hot path).
struct FStateFrame {
    FFrame Frame;                        // 0x0000 (size 0x48)
    UState* StateNode = nullptr;         // 0x0048
    std::uint32_t ProbeMask = 0;         // 0x0050
    std::uint16_t LatentAction = 0;      // 0x0054
    std::uint8_t bContinuedState = 0;    // 0x0056
    // 0x0057: one byte of C++ padding the compiler inserts to 8-align StateStack
    TArray<FPushedState> StateStack;     // 0x0058
    UClass* LocalVarsOwner = nullptr;    // 0x0068

    FStateFrame() = default;
    FStateFrame(UObject* object, UState* stateNode, std::uint8_t* code, std::uint8_t* locals);

    [[nodiscard]] FString Describe() const;
    void InitLocalVars(UObject* context);
    void ClearLocalVars();
};

// Intentionally no stock EX_* / CST_* byte-value table here, that's decompiler work not made for kuru :3

using Native = NativeFunctionPtr;

static_assert(sizeof(FOutParmRec) == 0x18);
static_assert(sizeof(FOutputDevice) == 0x18);  // FFrame's base: the 24 bytes before Node
// RL fork FFrame: fields shifted +0x10 vs stock UE3. Node/Object/Code/Locals are IDA-confirmed.
static_assert(offsetof(FFrame, Node) == 0x18);
static_assert(offsetof(FFrame, Object) == 0x20);
static_assert(offsetof(FFrame, Code) == 0x28);
static_assert(offsetof(FFrame, Locals) == 0x30);
static_assert(sizeof(FFrame) == 0x48);
static_assert(sizeof(FPushedState) == 0x18);
static_assert(sizeof(FStateFrame) == 0x70);  // inferred from the FFrame +0x10 shift
static_assert(offsetof(FStateFrame, StateNode) == 0x48);
static_assert(offsetof(FStateFrame, StateStack) == 0x58);

} // namespace Kuru::SDK
