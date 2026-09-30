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

`SomeParams` is the generated params struct for that function (from the SDK), or a plain type like `float`
for a single-parameter function.

## Cancel the call

Call `ctx.Block()` from a Pre hook to skip the original function:

```cpp
Kuru::Hooks::Pre("Function ...", [](Kuru::HookContext& ctx) {
    ctx.Block();  // the function never runs
});
```

## Read and edit output

After the call, out-parameters and the return value are filled in. In a Post hook read them with
`ctx.Params<T>()` (out params) and `ctx.Result<T>()` (return), and write to them to change what the caller
sees:

```cpp
Kuru::Hooks::Post("Function ...", [](Kuru::HookContext& ctx) {
    if (auto* r = ctx.Result<float>()) {
        *r = 0.0f;  // the caller gets 0
    }
});
```

## Notes

This works whether the game reaches the function through its event system, its bytecode interpreter, or a
native call - Kuru routes the hook to the layer that actually has the parameters. The mechanics differ per
layer. If you're curious, see
[editing parameters, internals](../concepts/08-editing-parameters-internals.md).
