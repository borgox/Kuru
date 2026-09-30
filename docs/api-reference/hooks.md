# Kuru::Hooks

Register callbacks that run when the game calls a function. See the [hooks guide](../modding/guides/hooks.md)
for the how and why.

A hook callback is `void(Kuru::HookContext&)`. The register functions return a `Kuru::HookHandle` you can pass
to `Remove`.

## Functions

`Pre(const char* functionFullName, callback) -> HookHandle`
Run the callback before the function. The name is the full function name, for example
`"Function TAGame.Ball_TA.OnHitGoal"`.

`Post(const char* functionFullName, callback) -> HookHandle`
Run the callback after the function.

`PreVia(const char* functionFullName, HookDispatch layer, callback) -> HookHandle`
Like `Pre`, but fire only at a specific dispatch layer instead of the automatic one.

`PostVia(const char* functionFullName, HookDispatch layer, callback) -> HookHandle`
The post version of `PreVia`.

`PreInternal(const char* functionFullName, callback) -> HookHandle`
Shorthand for `PreVia(name, HookDispatch::ProcessInternal, callback)`.

`PostInternal(const char* functionFullName, callback) -> HookHandle`
The post version of `PreInternal`.

`Remove(HookHandle handle)`
Remove a hook by its handle.

`SuppressOverrideWarnings()`
Stop Kuru warning this mod when it hooks a base-class function the game overrides.

## HookContext

Passed to every callback.

`Caller() -> UObject*`  the object the function was called on.
`Function() -> UFunction*`  the function being called.
`Params<T>() -> T*`  the parameters as a pointer to your params struct, or null if there are none.
`Result<T>() -> T*`  the return value, valid in post hooks.
`Block()`  from a pre hook, skip the original call.
`Dispatch() -> HookDispatch`  which layer this callback fired on.
