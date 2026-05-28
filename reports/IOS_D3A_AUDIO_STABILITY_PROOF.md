# iOS D3A Audio Stability Proof

Status: Initial D3A SFX audio/stability IPA produced and tested on physical iPhone; follow-up audio-category diagnostic IPA produced; SDL2_mixer-backed BGM build is in progress.

D3 first render remains complete. D3A is a stability/audio hardening pass for the repeatable foreground idle exit near the five-minute mark.

## Scope

D3A does not start touch controls, gameplay input polish, audio polish, SOR2-only pruning, App Store/TestFlight signing, or bundled game data.

The IPA still relies on the D2-staged data at:

```text
Library/Application Support/SORR
```

No `SorR.dat`, `data/`, FPG/WAV/OGG/SMK/PNG assets, prepared data, local ZIPs, logs, or build outputs are bundled or committed.

## Why This Pass Changed Direction

The idle-window diagnostics showed normal frame timing and no obvious file-handle or memory spike:

```text
dense_window_start ticks=250327 runtime_ms=249045
heartbeat=49 ticks=274425 runtime_ms=273143 stage=runtime-loop
frame_ms=16.667 max_jump=0
opened_files=1 x_files=0 max_x_files=0
rss_bytes approximately 90 MB
```

The suspicious signal was steady growth in the previous audio stub counters:

```text
heartbeat=24 audio_stub_zero=9478 audio_stub_minus_one=235
heartbeat=49 audio_stub_zero=13113 audio_stub_minus_one=308
```

D3A therefore replaces the pure iOS audio stub with a minimal SDL2 audio backend.

## Initial Physical D3A Result

The first D3A audio IPA was installed and tested on the physical iPhone. It still rendered and ran the real SoRR runtime from D2-staged data, and the visible diagnostics confirmed the minimal real SFX path is active:

```text
audio_init_attempts=1
audio_init_ok=1
audio_init_fail=0
audio_wav_load_ok=122
audio_wav_load_fail=0
audio_wav_play=304
audio_stub_minus_one=0
```

The app still exited in the same approximate five-minute foreground idle window:

```text
heartbeat=49 ticks=274472 runtime_ms=273127
rss_bytes approximately 145 MB
```

This means minimal real SFX audio works, but it did not fix the idle exit by itself. The remaining suspicious signals are the still-rising `audio_stub_zero` counter, missing or inert music/BGM behavior, and resource growth around the timed 240-300 second idle/demo window.

## D3A Audio Backend

The iOS `mod_sound` replacement now:

- initializes the real SDL audio subsystem,
- opens a real SDL audio device through the platform audio driver,
- keeps the Bennu `mod_sound` exports and globals intact,
- loads WAV effects through Bennu's `file_open` virtual file layer,
- converts WAV data to the opened SDL audio device format,
- queues WAV effects through `SDL_QueueAudio`,
- returns stable inert handles for unsupported music/OGG paths instead of repeated hard failures,
- uses the existing 64-bit pointer helper for `SP`/`P` sysproc pointer parameters,
- caps queued audio and clears excessive queued bytes as a safety guard,
- keeps Files-visible D3S diagnostics enabled.

This is not full SDL2_mixer/OGG/Vorbis support yet. Music may still be silent. The immediate goal is to stop hammering broken stub paths and test whether foreground idle stability improves.

## New Diagnostics

The visible diagnostics log remains:

```text
On My iPhone/SorrIOSShell/SORR_DIAGNOSTICS/ios_d3_runtime_stability_probe.txt
```

Heartbeat lines now include:

- `audio_init_attempts`
- `audio_init_ok`
- `audio_init_fail`
- `audio_wav_load_ok`
- `audio_wav_load_fail`
- `audio_wav_play`
- `audio_inert_handles`
- `audio_queue_clears`
- `audio_music_load_attempts`
- `audio_music_open_ok`
- `audio_music_open_fail`
- `audio_live_handles`
- `audio_live_wav`
- `audio_live_inert_wav`
- `audio_live_music`
- `audio_max_live_handles`
- `audio_zero_music_play`
- `audio_zero_music_control`
- `audio_zero_music_query`
- `audio_zero_wav_control`
- `audio_zero_wav_query`
- `audio_zero_wav_volume`
- `audio_zero_channel_effect`
- `audio_zero_play_wav_guard`
- `audio_music_last_status`
- `audio_music_last_path`
- legacy `audio_stub_zero`
- legacy `audio_stub_minus_one`

The follow-up diagnostic build splits `audio_stub_zero` into named categories so the next physical run can distinguish music play/control/query calls, WAV control/query/volume calls, channel effects, and guarded `PLAY_WAV` no-ops. It also logs the last music path and whether Bennu's file layer can open it.

## Local-Only Music Import Validation

The local-only Windows helper now validates that the import package includes prepared BGM/music data under `mod/music`. It refuses a source root with no `mod/music` audio files and verifies this ZIP entry in addition to `SorR.dat` and `mod/system.txt`:

```text
SORR_IMPORT/mod/music/1.ogg
```

The regenerated local-only package from `sorr-vita-master/data` contains 237 music/BGM files:

```text
Zip: out/local-only/SORR_IMPORT.zip
SHA256: 261190316E0D539546596738333D6752D69E209FBB8E6B7BF61B0B1DDBAB8A7D
```

This ZIP remains local-only. Do not commit, upload, attach, or bundle it.

## Artifact Target

GitHub Actions device artifact:

```text
ios-shell-d3a-audio-diagnostics-device-arm64
```

IPA inside artifact:

```text
build-products/SorrIOSShell-d3a-audio-diagnostics-adhoc.ipa
```

The workflow keeps the no-assets-in-IPA inspection:

```text
no SorR.dat
no data/
no .fpg/.wav/.ogg/.smk/.png assets
```

## CI Iteration Notes

- Run `26554204340` reached AppleClang for the device build but failed compiling `sorr_ios_mod_sound_stub.c`.
- Cause: the iOS-only audio replacement used Bennu global-access macros before the `mod_sound` globals-fixup array had a visible declaration.
- Fix: add a forward declaration for `__bgdexport(mod_sound, globals_fixup)` so `GLOEXISTS` and `GLODWORD` compile without changing desktop/x64 code.

## GitHub-Side Artifact Proof

```text
Actions run: 26554585872
Device artifact: ios-shell-d3a-audio-device-arm64
Artifact size: 724145 bytes
IPA: build-products/SorrIOSShell-d3a-audio-adhoc.ipa
Artifact-producing commit: d6785c7
Device job result: success
Simulator job result: success
Game data/assets bundled in IPA: no
```

This is not a physical stability pass yet. It is the installable D3A diagnostic build for the next iPhone test.

## Follow-Up Diagnostic Target

The next D3A diagnostic IPA keeps the same D2 data path and D3 render path but adds named audio zero categories, music file-open diagnostics, and live audio handle counters.

```text
Artifact: ios-shell-d3a-audio-diagnostics-device-arm64
IPA: build-products/SorrIOSShell-d3a-audio-diagnostics-adhoc.ipa
Purpose: identify which remaining audio no-op path climbs near the 240-300 second idle window
Game data/assets bundled in IPA: no
```

GitHub-side follow-up diagnostic artifact proof:

```text
Actions run: 26557322892
Device artifact: ios-shell-d3a-audio-diagnostics-device-arm64
Artifact size: 728042 bytes
IPA: build-products/SorrIOSShell-d3a-audio-diagnostics-adhoc.ipa
Artifact-producing commit: 0d3eb53
Device job result: success
Simulator job result: success
Game data/assets bundled in IPA: no
```

## Manual iPhone Test

1. Keep the D2-staged data on the iPhone.
2. If the iPhone data may lack `mod/music`, regenerate the local-only import package on Windows with `powershell -ExecutionPolicy Bypass -File tools\create_d2_import_package.ps1`, then transfer/extract the updated `out/local-only/SORR_IMPORT.zip` through the existing D2 Files route.
3. Download `ios-shell-d3a-audio-diagnostics-device-arm64` from GitHub Actions.
4. Extract the artifact on Windows.
5. Install `build-products/SorrIOSShell-d3a-audio-diagnostics-adhoc.ipa` through Sideloadly.
6. Open `SorrIOSShell`.
7. Confirm the real SoRR render still appears.
8. Note whether any audio is audible.
9. Leave the app foregrounded and untouched for 10-15 minutes.
10. If it exits, reopen it once.
11. Open Files: `On My iPhone -> SorrIOSShell -> SORR_DIAGNOSTICS`.
12. Copy or screenshot the last 40-60 lines of `ios_d3_runtime_stability_probe.txt`.
13. Include any `SDL_APP_*`, `dense_window_start`, `dense_window_end`, and heartbeat lines with the named audio counters and `audio_music_last_*` fields.

## Expected Result

Best case for the older SFX-only diagnostic build: the real SDL audio device opens, WAV effects can be queued, and the app idles 10-15 minutes without exiting. That endpoint is no longer enough for D3A, because the current target is audible BGM through SDL2_mixer.

Diagnostic case: if the app still exits, the visible log should show whether audio initialized, whether WAV loads/plays are happening, whether unsupported inert handles are stable, and whether the previous retry counters still climb near the 240-300 second window.

## D3A Music Backend Target

The physical D3A diagnostics showed that SFX/WAV playback works, `mod/music` files are present in the local-only import package, BGM path lookup works, and OGG files open through Bennu's file layer. The remaining failure is the iOS music backend itself: the previous `LOAD_SONG` path returned an `open-ok-inert` handle and `PLAY_SONG`/music controls were no-ops.

The next D3A build replaces that inert music path with SDL2_mixer:

- GitHub Actions builds SDL2_mixer for `iphoneos` arm64.
- SDL2_mixer is configured static, vendored, and OGG/Vorbis uses the built-in STB backend.
- The IPA still contains no game data or music assets.
- Runtime BGM stays private through the D2-staged `Library/Application Support/SORR/mod/music` data.
- `LOAD_SONG` now uses `Mix_LoadMUS_RW`.
- `PLAY_SONG`, fade, stop, pause, resume, volume, position, and playing-query calls route to SDL2_mixer.
- WAV/SFX also route through SDL2_mixer so there is one iOS audio backend instead of a queue-audio/SFX path plus inert music.

Artifact target:

```text
ios-shell-d3a-music-device-arm64
```

IPA target:

```text
build-products/SorrIOSShell-d3a-music-adhoc.ipa
```

Expected physical test:

1. Keep the D2-staged data on the iPhone, including `mod/music`.
2. Install `build-products/SorrIOSShell-d3a-music-adhoc.ipa` with Sideloadly.
3. Launch `SorrIOSShell`.
4. Confirm real SoRR rendering still appears.
5. Confirm whether BGM is audible.
6. Leave the app foregrounded and untouched for 10-15 minutes.
7. If it exits, reopen once and retrieve `On My iPhone/SorrIOSShell/SORR_DIAGNOSTICS/ios_d3_runtime_stability_probe.txt`.
8. Report the last 40-60 lines, especially `audio_music_last_status`, `audio_music_last_path`, `audio_music_open_ok`, `audio_music_open_fail`, and any `SDL_APP_*` lifecycle markers.

First SDL2_mixer CI result:

```text
Actions run: 26558533587
Artifact-producing commit: c733c2a
Simulator job result: success
Device job result: failed
Device artifact uploaded: ios-shell-d3a-music-device-arm64 logs only
```

Follow-up fix: force SDL2_mixer to configure as a static iOS device build with `BUILD_SHARED_LIBS=OFF`, pass the installed SDL2 CMake package through `SDL2_DIR`, and pass both SDL2 and SDL2_mixer prefixes to the iOS shell configure step.

## GitHub-Side D3A Music Artifact Proof

```text
Actions run: 26559098341
Device artifact: ios-shell-d3a-music-device-arm64
Artifact size: 791638 bytes
IPA: build-products/SorrIOSShell-d3a-music-adhoc.ipa
Artifact-producing commit: 3914295
Device job result: success
Simulator job result: success
Game data/assets bundled in IPA: no
```

The D3A music IPA is expected to render the real SoRR runtime from the existing D2-staged data and to play BGM through SDL2_mixer if `Library/Application Support/SORR/mod/music` is present on the iPhone.

## Physical D3A Music Result

The first SDL2_mixer music IPA crashed/exited much earlier than the previous idle-window issue:

```text
Tested run: 26559098341
Artifact: ios-shell-d3a-music-device-arm64
IPA: build-products/SorrIOSShell-d3a-music-adhoc.ipa
Observed previous marker: event=SDL_APP_TERMINATING ticks=30496 stage=runtime-loop
Previous older marker for comparison: heartbeat=49 ticks=274472 runtime_ms=273127
Interpretation: new D3A music-backend failure, not just the old five-minute idle exit
```

Follow-up change:

- Keep music enabled; do not return to inert handles.
- Load OGG/BGM files into an owned memory buffer before `Mix_LoadMUS_RW`.
- Keep that memory alive in the music handle until `Mix_FreeMusic`.
- Add visible heartbeat/event counters for music memory load, play attempts, play result, controls, queries, frees, halt calls, playing state, last handle, last pointer, and byte counts.
- Log `Mix_GetError` around music play/control paths.

Follow-up artifact target:

```text
ios-shell-d3a-music-diagnostics-device-arm64
build-products/SorrIOSShell-d3a-music-diagnostics-adhoc.ipa
```

## GitHub-Side D3A Music Diagnostic Artifact Proof

```text
Actions run: 26590593436
Device artifact: ios-shell-d3a-music-diagnostics-device-arm64
Artifact size: 793596 bytes
IPA: build-products/SorrIOSShell-d3a-music-diagnostics-adhoc.ipa
Artifact-producing commit: 2e27dda
Device job result: success
Simulator job result: success
Game data/assets bundled in IPA: no
```

## D3A Music Test Instructions

1. Keep the D2-staged data on the iPhone, including `mod/music`.
2. If the staged data may be stale, regenerate the local-only import ZIP on Windows with `powershell -ExecutionPolicy Bypass -File tools\create_d2_import_package.ps1`, then transfer/extract it through the existing D2 route.
3. Download `ios-shell-d3a-music-diagnostics-device-arm64` from the latest GitHub Actions run.
4. Extract the artifact on Windows.
5. Install `build-products/SorrIOSShell-d3a-music-diagnostics-adhoc.ipa` with Sideloadly.
6. Launch `SorrIOSShell`.
7. Confirm real SoRR rendering still appears.
8. Confirm whether BGM is audible.
9. Leave the app foregrounded and untouched for 10-15 minutes.
10. If it exits, reopen once and retrieve `On My iPhone -> SorrIOSShell -> SORR_DIAGNOSTICS -> ios_d3_runtime_stability_probe.txt`.
11. Report the last 40-60 lines, especially `audio_music_mem_ok`, `audio_music_play_attempts`, `audio_music_play_ok`, `audio_music_play_fail`, `audio_music_playing`, `audio_music_last_handle`, `audio_music_last_ptr`, `audio_music_last_bytes`, `audio_music_last_status`, and any `SDL_APP_*` lifecycle markers.

## Physical D3A Memory-Backed Music Result

The latest D3A music diagnostics log proves that real audio is no longer the remaining blocker:

```text
Tested artifact: ios-shell-d3a-music-diagnostics-device-arm64
Game renders: yes
BGM: working
SFX: working
Music memory path: working
Music play: working
Old inert music path: gone
```

Important log markers:

```text
audio_music_mem_ok=7
audio_music_mem_fail=0
audio_music_play_attempts=6
audio_music_play_ok=6
audio_music_play_fail=0
audio_music_playing=1
audio_music_last_status=play-ok
audio_music_last_path=mod/music/9a.ogg
audio_zero_music_play=0
audio_zero_music_control=0
audio_zero_music_query=0
audio_live_inert_wav=0
```

The app still reaches the same danger window:

```text
heartbeat=46 ticks=271380 runtime_ms=270112
stage=runtime-loop
audio_music_playing=1
audio_music_last_status=play-ok
```

Interpretation: D3A real BGM/SFX is working, and the remaining five-minute exit now looks more likely to be a timed attract/demo/menu/runtime path around 240-300 seconds than a missing or inert audio backend.

## D3S Runtime Window Diagnostic Target

The next D3S build keeps BGM and SFX enabled and adds Files-visible interpreter/runtime snapshots to the existing heartbeat log. It does not disable music and does not add touch controls.

New heartbeat fields include:

- `runtime_loops`, `runtime_frames`, `runtime_runs`
- `runtime_created`, `runtime_destroyed`, `runtime_snapshots`
- `runtime_last_proc`
- `runtime_snapshot` with a bounded sample of active process names, ids, statuses, frame percentages, code offsets, and priorities

Artifact target:

```text
ios-shell-d3s-runtime-window-diagnostics-device-arm64
```

IPA target:

```text
build-products/SorrIOSShell-d3s-runtime-window-diagnostics-adhoc.ipa
```

Physical test instructions:

1. Keep the D2-staged data on the iPhone, including `mod/music`.
2. Install `build-products/SorrIOSShell-d3s-runtime-window-diagnostics-adhoc.ipa` with Sideloadly.
3. Launch `SorrIOSShell`.
4. Confirm real SoRR rendering appears and BGM/SFX remain active.
5. Leave the app foregrounded and untouched for 10-15 minutes.
6. If it exits, reopen once.
7. Retrieve `On My iPhone -> SorrIOSShell -> SORR_DIAGNOSTICS -> ios_d3_runtime_stability_probe.txt`.
8. Report the last 60-100 lines, especially the dense-window heartbeats and any `SDL_APP_*` lifecycle event line with `runtime_snapshot`.

## GitHub-Side D3S Runtime Window Diagnostic Artifact Proof

```text
Actions run: 26592326534
Device artifact: ios-shell-d3s-runtime-window-diagnostics-device-arm64
Artifact size: 796018 bytes
IPA: build-products/SorrIOSShell-d3s-runtime-window-diagnostics-adhoc.ipa
Artifact-producing commit: 7f11519
Device job result: success
Simulator job result: success
Game data/assets bundled in IPA: no
```
## D3S Attract Lifecycle Diagnostic Target

Latest physical D3S feedback shows real audio is healthy: BGM is playing from the memory-backed OGG path, SFX works, and the zero music play/control/query counters are no longer active. The remaining five-minute foreground exit now appears tied to a timed runtime path after the game enters an attract/demo/gameplay-like scene.

Next diagnostic target:

- Artifact target: `ios-shell-d3s-attract-lifecycle-diagnostics-device-arm64`
- IPA target: `build-products/SorrIOSShell-d3s-attract-lifecycle-diagnostics-adhoc.ipa`

This build keeps BGM/SFX enabled and adds watchlist lifecycle diagnostics for active demo/runtime processes reported from the physical phone: `CONTROLADOR`, `BARRA_NEGRA`, `MELODIA`, `SOMBRA`, `ESTIRAMIENTO`, `BARRA_SEC_VIDA1`, `BARRA_VIDA1`, `EFECTO_POLVO`, `LETRA_NOMBRE`, and `MINI_CUADRO1`.

New log fields:

- `runtime_snapshot` now includes `watch=name:live:create:destroy` counts for the watchlist.
- `runtime_lifecycle` records a short create/destroy ring for recent process lifecycle events.
- `runtime_lifecycle ...` lines are mirrored directly into `Documents/SORR_DIAGNOSTICS/ios_d3_runtime_stability_probe.txt` for watched processes.
- fatal signal diagnostics, when catchable, append `signal=... runtime_snapshot=... runtime_lifecycle=...` to the visible diagnostics log.

Manual test:

1. Install `build-products/SorrIOSShell-d3s-attract-lifecycle-diagnostics-adhoc.ipa` with Sideloadly.
2. Launch the app with the existing D2-staged data.
3. Leave the app foregrounded and untouched for 10-15 minutes or until it exits.
4. Reopen once if it exits.
5. In Files, open `On My iPhone/SorrIOSShell/SORR_DIAGNOSTICS/ios_d3_runtime_stability_probe.txt`.
6. Report the last 80-120 lines, especially `dense_window_start`, heartbeats after `runtime_ms=240000`, `runtime_snapshot`, `runtime_lifecycle`, `runtime_lifecycle ...`, and any `signal=` line.
## GitHub-Side D3S Attract Lifecycle Diagnostic Proof

Status: produced successfully.

- Actions run: `26595939671`
- Artifact-producing commit: `f0553b4`
- Device artifact: `ios-shell-d3s-attract-lifecycle-diagnostics-device-arm64`
- IPA: `build-products/SorrIOSShell-d3s-attract-lifecycle-diagnostics-adhoc.ipa`
- Artifact size: `800173` bytes

The IPA remains asset-free and uses only the D2-staged private data on-device. It keeps real BGM/SFX enabled and adds watchlist process counts, recent create/destroy lifecycle events, direct `runtime_lifecycle ...` mirror lines, and catchable fatal-signal context to the Files-visible D3S diagnostics log.

Physical test instruction: install the IPA with Sideloadly, launch with the existing D2-staged data, leave foregrounded/untouched for 10-15 minutes or until it exits, reopen once if it exits, then retrieve `On My iPhone/SorrIOSShell/SORR_DIAGNOSTICS/ios_d3_runtime_stability_probe.txt`. Report the last 80-120 lines, especially entries after `runtime_ms=240000`, `watch=`, `runtime_lifecycle`, and any `signal=` line.
## D3S Demo Teardown Diagnostic Target

Latest on-device D3S diagnostics show the remaining exit coincides with attract/demo gameplay cleanup and a title/menu re-entry, while BGM and SFX remain healthy. The next build keeps audio enabled and instruments the transition more directly.

Target artifact:

- `ios-shell-d3s-demo-teardown-diagnostics-device-arm64`
- `build-products/SorrIOSShell-d3s-demo-teardown-diagnostics-adhoc.ipa`

Added diagnostics:

- expanded watchlist: `FASE1`, `DESCARGA_SISTEMA`, `SISTEMA_SONIDO`, `ASIGNADOR_ENEMIGO`, existing HUD/effect processes, and title/menu re-entry processes,
- `destroy_begin` lifecycle events before hierarchy and sibling pointers are rewritten,
- post-unlink `destroy` lifecycle events for comparison,
- family pointer ids and validity: father, son, small brother, big brother,
- `called_by` id and validity,
- priority/status/frame/code-offset context on watched lifecycle events.

Manual test: install the IPA with Sideloadly, leave the app foregrounded/untouched until it exits or 10-15 minutes pass, reopen once if needed, then report the last 100-150 lines of `On My iPhone/SorrIOSShell/SORR_DIAGNOSTICS/ios_d3_runtime_stability_probe.txt`.
## GitHub-Side D3S Demo Teardown Diagnostic Proof

Status: produced successfully.

- Actions run: `26598228033`
- Artifact-producing commit: `8017ff7`
- Device artifact: `ios-shell-d3s-demo-teardown-diagnostics-device-arm64`
- IPA: `build-products/SorrIOSShell-d3s-demo-teardown-diagnostics-adhoc.ipa`
- Artifact size: `798554` bytes

The IPA remains asset-free and uses only the D2-staged private data on-device. Use this build to capture the next physical-device teardown log around `FASE1`, `DESCARGA_SISTEMA`, `SISTEMA_SONIDO`, `ASIGNADOR_ENEMIGO`, HUD/effect process cleanup, and title/menu re-entry.
## D3S Title Re-Entry Diagnostic Target

Latest physical diagnostics show the demo teardown gets past enemy/stage cleanup and returns to a tiny title/intro/trophies process set before the app exits. Audio remains healthy and enabled, so this target focuses on process hierarchy integrity during title re-entry.

Target artifact:

- `ios-shell-d3s-title-reentry-diagnostics-device-arm64`
- `build-products/SorrIOSShell-d3s-title-reentry-diagnostics-adhoc.ipa`

Added diagnostics:

- explicit watchlist entries for `INTRO`, `MENU`, `TROPHIES_CALL`, `TROPHIES_CONTROL`, and `RESOLUCIONX`,
- direct `runtime_family_unlink ...` lines showing parent/child/sibling ids before and after `instance_destroy` hierarchy unlinking,
- continued `destroy_begin` and post-unlink `destroy` lifecycle lines with called-by validity,
- Files-visible README updated to request `runtime_family_unlink` lines after a crash.

Manual test: install the IPA with Sideloadly, leave the app foregrounded/untouched until it exits or 10-15 minutes pass, reopen once if needed, then report the last 150-200 lines of `On My iPhone/SorrIOSShell/SORR_DIAGNOSTICS/ios_d3_runtime_stability_probe.txt`.
## GitHub-Side D3S Title Re-Entry Diagnostic Proof

Status: produced successfully.

- Actions run: `26599649375`
- Artifact-producing commit: `5d63a6d`
- Device artifact: `ios-shell-d3s-title-reentry-diagnostics-device-arm64`
- IPA: `build-products/SorrIOSShell-d3s-title-reentry-diagnostics-adhoc.ipa`
- Artifact size: `799257` bytes

The IPA remains asset-free and uses only the D2-staged private data on-device. Use this build to capture the next physical-device title re-entry log around `MENU`, `INTRO`, `TROPHIES_CALL`, `TROPHIES_CONTROL`, `RESOLUCIONX`, and any `runtime_family_unlink` lines.
## D3S LAYER_INTRO Diagnostic Target

Latest physical D3S feedback points beyond menu/trophies teardown and toward the intro/title animation path: `runtime_last_proc=LAYER_INTRO`, `last_create=LAYER_INTRO`, `last_destroy=OSCURECE_PANTALLA`, with `LAYER_INTRO` and `INTRO_PRINCIPIO` live in the final sample. BGM and SFX remain healthy.

Target artifact:

- `ios-shell-d3s-layer-intro-diagnostics-device-arm64`
- `build-products/SorrIOSShell-d3s-layer-intro-diagnostics-adhoc.ipa`

Added diagnostics:

- explicit watchlist entries for `LAYER_INTRO` and `INTRO_PRINCIPIO`,
- render-object create/destroy counters in heartbeat lines,
- direct `runtime_render_event ...` lines for watched process render-object create/destroy,
- guard logging for render callbacks whose backing process pointer is no longer live,
- continued family unlink and lifecycle diagnostics for title re-entry processes.

Manual test: install the IPA with Sideloadly, leave the app foregrounded/untouched until it exits or 10-15 minutes pass, reopen once if needed, then report the last 150-200 lines of `On My iPhone/SorrIOSShell/SORR_DIAGNOSTICS/ios_d3_runtime_stability_probe.txt`.
## GitHub-Side D3S LAYER_INTRO Diagnostic Proof

Status: produced successfully.

- Actions run: `26601081243`
- Artifact-producing commit: `79342fe`
- Device artifact: `ios-shell-d3s-layer-intro-diagnostics-device-arm64`
- IPA: `build-products/SorrIOSShell-d3s-layer-intro-diagnostics-adhoc.ipa`
- Artifact size: `799927` bytes

The IPA remains asset-free and uses only the D2-staged private data on-device. Use this build to capture the next physical-device log around `LAYER_INTRO`, `INTRO_PRINCIPIO`, `OSCURECE_PANTALLA`, `runtime_render_event`, and `runtime_family_unlink`.
## D3S Enemy/HUD Crash Diagnostic Target

Latest physical diagnostics captured `signal=11 ticks=275296 stage=runtime-loop`, confirming a bad memory access. Audio remains healthy, and the final context points back into active attract/demo gameplay with `runtime_last_proc=ENEMIGO` and nearby lifecycle/render activity around enemy/HUD/effect processes.

Target artifact:

- `ios-shell-d3s-enemy-hud-diagnostics-device-arm64`
- `build-products/SorrIOSShell-d3s-enemy-hud-diagnostics-adhoc.ipa`

Added diagnostics:

- explicit watchlist entries for `ENEMIGO` and `ESCRIBE_ENEMIGO`,
- continued watch coverage for `BARRA_NEGRA`, `BARRA_VIDA1`, `BARRA_SEC_VIDA1`, `MINI_CUADRO1`, `EFECTO_POLVO`, `SOMBRA`, and `ESTIRAMIENTO`,
- signal-time dump of last lifecycle event, last family unlink, last render event, last process pointer, and current process pointer,
- existing render callback guards remain enabled.

Manual test: install the IPA with Sideloadly, leave the app foregrounded/untouched until it exits or 10-15 minutes pass, reopen once if needed, then report the last 150-250 lines of `On My iPhone/SorrIOSShell/SORR_DIAGNOSTICS/ios_d3_runtime_stability_probe.txt`.
