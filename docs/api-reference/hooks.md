# Kuru::Hooks

Register callbacks that run when the game calls a function. See the [hooks guide](../modding/guides/hooks.md)
for the how and why.

A hook callback is `void(Kuru::HookContext&)`. The register functions return a `Kuru::HookHandle` you can pass
to `Remove`.

A zero handle means registration failed. Names identify a particular reflected `UFunction`, hooks do not
automatically cover overrides or C++ calls that bypass the intercepted dispatch layers. Callbacks run
on the thread that reaches the game function. Register within an owned mod callback (normally `OnLoad`)
so teardown can associate the hook with the mod.

Current wrappers require API v4. Missing functions (including classes not yet loaded), missing host
entries, or shutdown can reject registration. Check the handle and log; there is no automatic retry.
The Functions tab shows ProcessEvent calls, but `Any` does not fire at that layer. Select ProcessEvent
explicitly when observing that path. See [hook diagnostics](../modding/guides/hooks.md#my-hook-never-fires).

With host API v4, Kuru releases callbacks and their captured values when hooks,
event listeners, or commands are removed or replaced, or their mod is unloaded.
A callback already in progress stays alive until it returns.

## Types

`HookHandle` is a `std::uint64_t`; zero is invalid. `Events::ListenerHandle` aliases it. Module-event
handles must go through `Events::Unregister<EventT>`, not `Hooks::Remove`.

| `HookDispatch` | Value | Selection |
| --- | --- | --- |
| `ProcessEvent` | `0` | Explicit event entry point |
| `ProcessInternal` | `1` | Explicit script interpreter entry point |
| `CallFunction` | `2` | Explicit bytecode call dispatcher |
| `Any` | `0xffffffff` | Canonical layer selected from function flags |

`Any` dedup uses the thread and function pointer, not the caller object. Nested calls of that function
on a different object are suppressed too. Use an explicit layer if you need those invocations.

## Functions

### `Pre(const char* functionFullName, callback) -> HookHandle`

Run the callback before the function. The name is the full function name, for example
`"Function TAGame.Ball_TA.OnHitGoal"`.

### `Post(const char* functionFullName, callback) -> HookHandle`

Run the callback after the function.

### `PreVia(const char* functionFullName, HookDispatch layer, callback) -> HookHandle`

Like `Pre`, but fire only at a specific dispatch layer instead of the automatic one.

### `PostVia(const char* functionFullName, HookDispatch layer, callback) -> HookHandle`

The post version of `PreVia`.

### `PreInternal(const char* functionFullName, callback) -> HookHandle`

Shorthand for `PreVia(name, HookDispatch::ProcessInternal, callback)`.

### `PostInternal(const char* functionFullName, callback) -> HookHandle`

The post version of `PreInternal`.

### `Remove(HookHandle handle)`

Remove a hook by its handle.

### `SuppressOverrideWarnings()`

Stop Kuru warning this mod when it hooks a base-class function the game overrides.

## HookContext

Passed to every callback.

- `Caller() -> UObject*`: the object the function was called on.
- `Function() -> UFunction*`: the function being called.
- `Params<T>() -> T*`: a borrowed view of the parameter buffer, possibly null if unavailable. The cast
  does not check its type or size, and non-null does not prove the full expected buffer is present.
- `Result<T>() -> T*`: the return slot, possibly null. Produced values are read in post hooks.
- `Block()`: from a pre hook, request skipping the original call. CallFunction needs successful argument
  evaluation to locate the end of the arguments; blocking is not supported there if that fails.
- `Dispatch() -> HookDispatch`: which layer this callback fired on.

- `Frame() -> FFrame*`: frame view for ProcessInternal/CallFunction, null for ProcessEvent.
- `CallerAs<T>() -> T*`: unchecked cast of `Caller()`.
  `RawParams()` / `RawResult()` return the untyped pointers; `Blocked()` reads the block flag.

Never retain context, frame, parameter, or result pointers beyond the callback. See
[editing parameters](../modding/guides/editing-parameters.md) for native argument limits.

Post hooks still run after a call is blocked. Check `Blocked()` before assuming the original produced
out-parameters or a return value; return initialization differs between dispatch paths.

Do not inspect/edit/block CallFunction captures with `ParmsSize > 1024`. An early bounds break can
leave a success-looking argument-end pointer inside the bytecode argument stream; blocking can then
resume at the wrong position. This needs a code fix, not just a null-pointer check.
