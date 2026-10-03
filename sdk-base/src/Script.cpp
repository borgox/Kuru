#include <Core/Script.hpp>

#include <cstring>

namespace Kuru::SDK {

FFrame::FFrame(UObject* object, UStruct* node, std::uint8_t* code, std::uint8_t* locals)
    : Node(node), Object(object), Code(code), Locals(locals) {}

void FFrame::Serialize(FArchive& archive) {
    archive << Node;
    archive << Object;
}

std::int32_t FFrame::ReadInt() {
    std::int32_t value = 0;
    std::memcpy(&value, Code, sizeof(value));
    Code += sizeof(value);
    return value;
}

float FFrame::ReadFloat() {
    float value = 0.0f;
    std::memcpy(&value, Code, sizeof(value));
    Code += sizeof(value);
    return value;
}

FName FFrame::ReadName() {
    FName value;
    std::memcpy(&value, Code, sizeof(value));
    Code += sizeof(value);
    return value;
}

UObject* FFrame::ReadObject() {
    UObject* value = nullptr;
    std::memcpy(&value, Code, sizeof(value));
    Code += sizeof(value);
    return value;
}

std::uint16_t FFrame::ReadWord() {
    std::uint16_t value = 0;
    std::memcpy(&value, Code, sizeof(value));
    Code += sizeof(value);
    return value;
}

std::uint16_t FFrame::ReadCodeSkipCount() {
    return ReadWord();
}

std::int32_t FFrame::ReadVariableSize() {
    return ReadInt();
}

FString FFrame::GetStackTrace() const {
    return Node ? FString::FromAnsi(Node->GetFullName()) : FString();
}

FStateFrame::FStateFrame(UObject* object, UState* stateNode, std::uint8_t* code, std::uint8_t* locals)
    : Frame(object, stateNode, code, locals), StateNode(stateNode) {}

FString FStateFrame::Describe() const {
    return Frame.GetStackTrace();
}

void FStateFrame::InitLocalVars(UObject* context) {
    LocalVarsOwner = context ? context->GetClass() : nullptr;
}

void FStateFrame::ClearLocalVars() {
    LocalVarsOwner = nullptr;
}

} // namespace Kuru::SDK
