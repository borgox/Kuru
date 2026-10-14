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
- `ctx.Params<T>()` - a borrowed view of the parameter block, possibly null when unavailable.
- `ctx.Result<T>()` - the return value (Post hooks).
- `ctx.Block()` - from a Pre hook, skip the original call.

```cpp
Kuru::Hooks::Post("Function TAGame.Ball_TA.OnHitGoal", [](Kuru::HookContext& ctx) {
    if (auto* caller = ctx.Caller()) {
        Kuru::Info("goal hook caller: " + caller->GetName());
    }
});
```

Editing parameters, catching output, and cancelling get their own page - see
[editing parameters](editing-parameters.md).

## Finding a function to hook

You rarely know the exact name up front. Kuru has two tools for this in the F2 menu:

- **Functions tab** - turn on recording to capture calls passing through ProcessEvent, with a copied
  parameter block. This is not every VM/native call. Do the thing you want to react to and watch what fires;
  filter by name to narrow it down.
- **Objects tab** - search live objects and classes, and browse their functions and properties.

Copy the full name from there into `Hooks::Pre` / `Hooks::Post`.

## My hook never fires

Check for a nonzero registration handle and read `kuru.log`. A zero handle can mean an unknown or
not-yet-loaded function, an older host than the current wrapper supports (API v4), or shutdown.
Registration does not retry automatically. If a class loads later, retry from an owned callback once
it is present; avoid registering duplicates or retrying every frame indefinitely.

The Functions tab records ProcessEvent traffic. Automatic (`Any`) hooks choose ProcessInternal or
CallFunction, never ProcessEvent. A native reached only through ProcessEvent can therefore appear
in the scanner while its automatic hook stays silent. To observe a recorded path explicitly:

```cpp
auto handle = Kuru::Hooks::PreVia("Function TAGame.Ball_TA.OnHitGoal",
    Kuru::HookDispatch::ProcessEvent, [](Kuru::HookContext& ctx) {
        Kuru::Info("observed ProcessEvent, dispatch=" +
                   std::to_string(static_cast<std::uint32_t>(ctx.Dispatch())));
    });
```

Substitute the name you recorded. This diagnostic targets a particular path, not every invocation.
The template's automatic goal hook has separately passed the live freeplay test.

The scanner copies bytes, not the pointed-to contents of strings/arrays or the lifetime of objects.
Recorded data can become stale. Its **Resend call** feature does not make such a capture safe to replay.

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

A call can pass through several VM dispatch layers. `Any` hooks use a canonical layer selected from
function flags and suppress nested entries with the same `UFunction*` on the same thread, even if the
nested caller is another object. Explicit layers bypass that rule. Some paths bypass the selected layer;
explicit `PreVia`/`PostVia` hooks can target the actual path. For the details, see
[function dispatch layers](../concepts/05-function-dispatch-layers.md).
