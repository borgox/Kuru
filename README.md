# Kuru

Kuru is a C++ Rocket League SDK generator and mod-loader workspace.

The project is intentionally split into layers:

- `libs/KuruCommon` — shared memory, pattern, layout, and dump-schema utilities.
- `libs/KuruDump` — injected runtime dumper that captures UE3/RL reflection data.
- `libs/KuruSDKGen` — offline SDK generator from `dump.json` + `sdk-base`.
- `sdk-base` — hand-authored, runtime-verified UE3/RL base declarations and helpers.
- `libs/KuruLoader` — injected mod loader and hook runtime.
- `libs/KuruModSDK` — public mod-facing API headers.
- `mods/ExampleMod` — minimal sample mod target.

