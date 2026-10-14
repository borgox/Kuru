# Kuru::IMod and KURU_EXPORT_MOD

Every mod is a class implementing `Kuru::IMod`, exported with `KURU_EXPORT_MOD`. See
[what is a mod](../modding/getting-started/what-is-a-mod.md).

## IMod

Override the callbacks you need. All of them are optional.

### `bool OnLoad()`

Called on initial load, re-enable, and after Reload creates a fresh instance. Register hooks,
commands, and CVars here so they are registered again on re-enable. Initial loading runs on the bootstrap
worker, while re-enable/reload runs through Kuru's game-thread dispatcher. Use `Execute` for game work.

Returning false on initial load removes registrations and pending tasks, destroys the partial instance,
and releases its DLL. It does not call `OnUnload`. A false return or C++ exception during re-enable/reload
also removes registrations but leaves the instance disabled for a later retry. Initial `OnLoad` and
factory calls do not have the same C++ exception wrapper; return false for expected initial failures.

### `void OnUpdate(float deltaSeconds)`

Called for enabled mods when the dispatcher pumps an update. `Present` marks an update pending. A later
`ProcessEvent` detour consumes it. `deltaSeconds` measures time between these updates, not a guaranteed
fixed simulation step. See [threading](../modding/guides/threading.md) for the thread-identity limitation.

### `void OnRender()`

Called for enabled mods during ImGui frame building on the window thread, even when F2/F3 are
closed. Call `Gui::EnsureContext` before ImGui. Drawing is submitted separately from the render thread.

### `void OnUnload()`

Called before disabling, reloading an enabled instance, or ejecting an enabled mod. Stop and join
mod-owned workers and release external resources here. Host registrations and pending tasks are
cleaned up automatically. If `OnUnload` throws an ordinary C++ exception, Kuru logs it and continues
host cleanup. Your mod must still release its own resources. Disabled instances do not receive a second
`OnUnload` on reload/eject, and partial failed loads receive none.

Disable keeps the instance alive. Reload destroys it and creates a new instance from the already
loaded DLL. To load new compiled code, eject Kuru, replace the DLL, and inject again.

## Failure handling

Mods are native code inside Rocket League. Error handling does not roll back memory corruption.
Exception handling depends on the callback:

| Entry point | Ordinary C++ exception handling | Access-violation guard |
| --- | --- | --- |
| Initial factory / `OnLoad` | No dedicated wrapper | No |
| Re-enable/reload `OnLoad` and reload factory | Caught and logged; stays disabled | No |
| `OnUnload` | Caught and logged; host cleanup continues | No |
| `OnUpdate`, Execute/timeouts | Caught per invocation without a log line | No |
| `OnRender` | Caught around the renderer batch without a log line; later mods can be skipped for that frame | No |
| Console handler | No dedicated wrapper | No |
| Hook / module-event listener | SEH wrapper logs faults; normal C++ unwinding is not guaranteed by it | Yes, limited to the invocation |

Do not block initial `OnLoad` waiting for queued Execute work: detours are enabled only after loading
finishes, so that work cannot run while loading waits for it. Return false for an expected initial-load
failure. Release partial resources yourself and keep destructors nonthrowing.

The first dispatcher delta is zero. Immediate tasks run before due timers and `OnUpdate`. Work added
to a queue after its batch has been collected waits for a later pump; an immediate task that schedules
a due timeout can still have it collected by the timeout stage of the same pump.

## Multiple mods

Directory iteration does not define a load order or dependency system. Do not assume another mod's
listeners are registered during your `OnLoad`. Coordinate readiness explicitly or dispatch later.
Commands/CVars share names across mods, and data directories share the requested mod name. Prefix
names and agree on a shared event payload/version between cooperating mods.

## KURU_EXPORT_MOD

### `KURU_EXPORT_MOD(ModType, "Name", "Author", "Version")`

Exports the mod so Kuru can load it. Put it once at file scope. `ModType` is your `IMod` class, and the strings
show up in the Mods tab.

### Host API versions

| API version | Relevant behavior |
| --- | --- |
| Before v3 | Legacy task functions have no canceled-holder cleanup callback |
| v3 | Adds owned task storage and cleanup on execution/cancellation |
| v4 (current) | Adds owned hook, module-listener, and command storage cleanup |

The macro exports the version the mod was built against. A v4-built mod is refused by a host below
v4 even if it uses only older features. An accepted API version does not check compatibility with game
layouts or ImGui. `HostApi`, runtime binding setters, and export entry points are integration APIs;
ordinary mods should use the wrappers rather than calling or changing these tables directly.
