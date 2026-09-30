# Updating for a new Rocket League version

When Rocket League updates, most of Kuru keeps working because it resolves the engine at runtime. The usual
outcome is that only the SDK needs regenerating. This is the checklist to confirm that, and to catch the rare
cases where more is needed.

## Work down this list

1. **Inject and read the bootstrap log.** Every resolver should report success: the vtable layers, `GNames`
   and `GObjects`, `ProcessInternal`, `GNatives`, and the interpreter globals. If one failed, it is almost
   always a pattern; fix it with [finding patterns](finding-patterns.md).
2. **Check reflection in the Objects tab.** Search a known class, inspect an instance, and confirm names,
   classes, and properties come out sane. Garbage here means a core layout moved; see
   [verifying layouts](verifying-layouts.md).
3. **Check hooks in the Functions tab.** Turn on recording and confirm calls are logged with sane parameters.
4. **Regenerate the SDK.** Offsets shift on nearly every update, so [regenerate](regenerating-the-sdk.md) and
   rebuild your mods against it.

## What each symptom means

| Symptom | Cause | Fix |
| --- | --- | --- |
| A resolver fails in the bootstrap log | a byte pattern stopped matching | [finding patterns](finding-patterns.md) |
| Reflection or object names are garbage | a core struct offset moved | [verifying layouts](verifying-layouts.md) |
| Fields read wrong values, hooks see wrong params | stale SDK offsets | [regenerate the SDK](regenerating-the-sdk.md) |
| Everything works | nothing broke this update | regenerate the SDK to be safe |

Most updates are the last two rows: the loader self-resolves, and regenerating the SDK is the only real work.
