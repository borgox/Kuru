# Updating for a new Rocket League version

Runtime resolution can tolerate some code movement, but a game update can change layouts, resolver
assumptions, or function behavior. Use this checklist without assuming regeneration alone is sufficient.

## Work down this list

1. **Inject and read the bootstrap log.** Every resolver should report success: the vtable layers, `GNames`
   and `GObjects`, the game allocator, `ProcessInternal`, `GNatives`, and interpreter globals. Check for
   fallback RVAs and partial hook installation. Identify the failed stage using
   [finding patterns](finding-patterns.md), not every resolver failure is a changed signature.
2. **Check reflection in the Objects tab.** Search a known class, inspect an instance, and confirm names,
   classes, and properties come out sane. Garbage here means a core layout moved; see
   [verifying layouts](verifying-layouts.md).
3. **Check hooks in the Functions tab.** Turn on recording and confirm calls are logged with sane parameters.
4. **Regenerate the SDK** and [rebuild mods](regenerating-the-sdk.md) against the new headers. Rebuild the
   loader too if its typed cursor helpers or other generated-header dependencies changed.
5. **Test the release workflow.** On a fresh Kuru data directory, generate/build/install the template mod,
   run its command and goal hook in offline freeplay, disable/re-enable, recreate its instance with Reload,
   and eject. Verify that actual DLL replacement uses eject/reinject, not just instance Reload.

## What each symptom means

| Symptom | Possible cause | Next check |
| --- | --- | --- |
| A resolver fails | pattern, layout, readiness, or pointer validation changed | [finding patterns](finding-patterns.md) |
| Reflection or names are garbage | core layout or globals resolution is wrong | [verifying layouts](verifying-layouts.md) |
| Generated `sizeof` assertions fail | Reflected field sizes or handwritten bases no longer match | Check delegate footprints and engine-base offsets; keep the assertions and regenerate after fixing the SDK |
| A field or parameter reads incorrectly | stale layout, wrong function/layer, or unsupported argument handling | verify metadata and hook path before editing |
| A smoke test passes | the exercised paths worked | test the remaining paths; don't infer full compatibility |

Record the game build, loader/SDK version, successful checks, and unresolved warnings with the release.

## Useful log fragments

| Log fragment | Meaning |
| --- | --- |
| `ProcessEvent resolved via vtable[67]` | An address was obtained; hook installation is a separate step |
| `CallFunction (slot 76) detour installed` | The bytecode call detour installed |
| `ProcessInternal NOT resolved` | Automatic script-body hooks lack their selected layer |
| `interpreter globals: runtime resolve FAILED` | Build-specific RVAs were supplied instead of runtime results |
| `hook callback faulted (suppressed)` | A callback fault was contained, not proof the mod is healthy |

See [troubleshooting](../troubleshooting.md) for user-facing loading errors.
