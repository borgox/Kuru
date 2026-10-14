# Editing parameters

A Pre hook can change what a function receives, or stop it from running. A Post hook can read what it
produced and change that. Kuru handles the machinery - you read and write through the context.

## Change an input

Edit `ctx.Params<T>()` in a Pre hook and the function runs with your values:

```cpp
Kuru::Hooks::Pre("Function ...", [](Kuru::HookContext& ctx) {
    if (auto* p = ctx.Params<SomeParams>()) {
        p->Speed = 2.0f;  // the game sees 2.0
    }
});
```

This is schematic: replace the function name and `SomeParams` with a matching generated params struct.
A plain `float` view is appropriate only after confirming the field is a float at offset zero. The
cast itself does not check the buffer layout or size, and all pointers expire with the callback.

## Cancel the call

Call `ctx.Block()` from a Pre hook to request skipping the original function:

```cpp
Kuru::Hooks::Pre("Function ...", [](Kuru::HookContext& ctx) {
    ctx.Block();  // see dispatch-specific limits below
});
```

## Read and edit output

After the call, out-parameters and the return value are filled in. In a Post hook read them with
`ctx.Params<T>()` (out params) and `ctx.Result<T>()` (return), and write to them to change what the caller
sees. Post hooks also run for blocked calls; check `ctx.Blocked()` before assuming a value was produced:

```cpp
Kuru::Hooks::Post("Function ...", [](Kuru::HookContext& ctx) {
    if (auto* r = ctx.Result<float>()) {
        *r = 0.0f;  // the caller gets 0
    }
});
```

## Notes

Support depends on the path. ProcessEvent exposes its supplied block; ProcessInternal exposes frame
locals. CallFunction reconstructs supported arguments into a 1024-byte scratch buffer and tracks at most
32 argument slots. If evaluation fails, `Params` can be null and `Block` cannot skip that inline call.
Non-null alone does not guarantee a complete block for an oversized function.

Do not inspect/edit/block CallFunction captures with `ParmsSize > 1024`. An early bounds break can
leave an incorrect argument-end pointer without reporting failure, so blocking may corrupt bytecode
execution. Inspect `ctx.Dispatch()` and the function's `ParmsSize` before relying on this path.

Argument reconstruction evaluates expressions and may repeat side effects when the original call
evaluates them again. Strings, arrays, and other pointer-bearing values are not deeply copied, and
the native reinvocation path applies only to eligible functions. Verify simple value parameters first.
See
[editing parameters, internals](../concepts/08-editing-parameters-internals.md).

## A concrete parameter layout

In the generated TAGame package, `APlayerController_Menu_TA` contains a nested `PlayerTick_Params`
with `float DeltaTime` at offset zero. This compile-checked example clamps a value at the explicit
interpreter layer; parameter editing itself has not been live-tested by this documentation audit:

```cpp
Kuru::Hooks::PreInternal("Function TAGame.PlayerController_Menu_TA.PlayerTick",
    [](Kuru::HookContext& ctx) {
        using Params = Kuru::RL::APlayerController_Menu_TA::PlayerTick_Params;
        if (auto* p = ctx.Params<Params>(); p && p->DeltaTime > 0.1f) {
            p->DeltaTime = 0.1f;
        }
    });
```

Verify the generated declaration for your game build. The placeholder examples above illustrate
other operations; they are not independently tested recipes for blocking arbitrary functions or
changing an arbitrary return type.
