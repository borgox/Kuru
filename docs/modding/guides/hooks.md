# Hooks

A hook runs your code when the game calls one of its functions. You can read the call, change it, or cancel
it. Hooks are how a mod reacts to almost anything the game does.

## Pre and Post

- `Kuru::Hooks::Pre(name, callback)` runs *before* the function. You see its parameters, and can edit them
  or cancel the call.
- `Kuru::Hooks::Post(name, callback)` runs *after* the function. You see the result and any output.

```cpp
Kuru::Hooks::Post("Function TAGame.Ball_TA.OnHitGoal", [](Kuru::HookContext& ctx) {
    Kuru::Info("goal scored");
});
```

`name` is the function's full name, in the form `Function <Package>.<Class>.<Name>`.

## The context

Your callback gets a `Kuru::HookContext&`. Through it you read and edit the call:

- `ctx.Caller()` - the object the function was called on.
- `ctx.Params<T>()` - the parameters as a pointer to your params struct, or `nullptr` if there are none.
- `ctx.Result<T>()`- the return value (Post hooks).
- `ctx.Block()` - from a Pre hook, skip the original call.

```cpp
Kuru::Hooks::Pre("TAGame.PlayerInput_TA.PlayerInput", [](Kuru::HookContext& ctx) {
    if (const float* dt = ctx.Params<float>()) {
        Kuru::Info("frame delta: " + std::to_string(*dt));
    }
});
```

Editing parameters, catching output, and cancelling get their own page - see
[editing parameters](editing-parameters.md).

## Finding a function to hook

You rarely know the exact name up front. Kuru has two tools for this in the F2 menu:

- **Functions tab** - turn on recording and Kuru lists every function the game calls, live, with its
  parameters. Do the thing you want to react to (score a goal, boost, open a menu) and watch what fires;
  filter by name to narrow it down.
- **Objects tab** - search live objects and classes, and browse their functions and properties.

Copy the full name from there into `Hooks::Pre` / `Hooks::Post`.
<!-- if only you could view the game's source code this process would be easier #foreshadowing :eyes: -->
## Removing a hook

`Pre` / `Post` return a handle. Keep it if you want to remove the hook later:

```cpp
Kuru::HookHandle handle = Kuru::Hooks::Post("Function TAGame.Ball_TA.OnHitGoal", cb);
// later:
Kuru::Hooks::Remove(handle);
```

## A note on overrides

Some functions are declared on a base class but the game actually calls an override on a subclass. Kuru
warns you when it spots this, so you can hook the name that really fires. To silence it once you're sure,
call `Kuru::Hooks::SuppressOverrideWarnings()` in your mod.

## Going deeper

A call can pass through several VM dispatch layers. Kuru picks the right one so a name-based hook fires
once, with its parameters. For the details, see
[function dispatch layers](../concepts/05-function-dispatch-layers.md).
