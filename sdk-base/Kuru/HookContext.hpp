#pragma once

#include <cstdint>

namespace Kuru::SDK {
class UObject;
class UFunction;
struct FFrame;
}

/* One abstraction over every VM dispatch layer
 * A hook callback receives a HookContext& and reads/edits the call through it, so PE (flat params
 * blob) and PI (params in the frame's Locals) look the same to a modder.
*/
namespace Kuru {

// RL is a Psyonix UE3 FORK, so these signatures are not the stock UE3 ones, indices and arg shapes are confirmed
enum class HookDispatch : std::uint32_t {
    ProcessEvent = 0,          // vtable 67 - UFunction in arg1. params is a flat blob (https://tinyurl.com/36tn4mv3)
    ProcessInternal = 1,       // NON-virtual in RL - not in the vtable, not hookable by index (reserved)
    CallFunction = 2,          // vtable 76 - bytecode call dispatch. UFunction in arg3, params in the frame
    /*Default for name-based hooks: fire on whichever layer dispatches the function, deduped to one
    * call per native invocation (nested layers of the same function are suppressed).
    */
    Any = 0xffffffff,
};

struct HookContext {
    HookDispatch dispatch;
    SDK::UObject* caller;
    SDK::UFunction* function;
    void* params;          // PE: the Parms blob. PI: the frame's Locals. May be null.
    void* result;          // return-value slot (may be null)
    SDK::FFrame* frame;     // PI/CF only (call chain via frame->PreviousFrame) else null
    bool block;            // Pre hooks: set via Block() to skip the original call (where supported)

    [[nodiscard]] SDK::UObject* Caller() const { return caller; }
    [[nodiscard]] SDK::UFunction* Function() const { return function; }
    [[nodiscard]] HookDispatch Dispatch() const { return dispatch; }
    [[nodiscard]] void* RawParams() const { return params; }
    [[nodiscard]] void* RawResult() const { return result; }
    [[nodiscard]] SDK::FFrame* Frame() const { return frame; }

    // Typed views,  pass the generated _Params struct, the object type, etc.
    template <typename T> [[nodiscard]] T* Params() const { return reinterpret_cast<T*>(params); }
    template <typename T> [[nodiscard]] T* Result() const { return reinterpret_cast<T*>(result); }
    template <typename T> [[nodiscard]] T* CallerAs() const { return reinterpret_cast<T*>(caller); }

    void Block() { block = true; }
    [[nodiscard]] bool Blocked() const { return block; }
};

} // namespace Kuru
