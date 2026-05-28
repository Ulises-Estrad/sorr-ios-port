# iOS D3 Stability Follow-Up

Status: D3A minimal real-audio IPA produced and physically tested; follow-up audio-category diagnostic IPA produced by GitHub Actions and ready for physical iPhone test.

D3 first render remains complete. This pass hardens the already-rendering D3 build after a repeatable physical-device idle exit was observed at about five minutes.

## Scope

This is D3 stability only.

No touch controls, gameplay input, audio polish, SOR2-only pruning, App Store/TestFlight signing, or bundled game data are part of this pass.

## Change Summary

The D3S iOS build now:

- disables the iOS idle timer through SDL before initialization,
- keeps the D2 data preflight unchanged,
- writes the original first-render probe log,
- writes a persistent D3 runtime stability log,
- mirrors the stability log to the Files-visible Documents area,
- copies the latest app-private stability log to the visible mirror on startup,
- records the previous stability log's last marker on the next launch,
- starts a 10-second heartbeat before runtime handoff,
- logs the current D3 stage and resident memory in each heartbeat,
- logs one-second dense heartbeats from runtime `240000` ms through `330000` ms,
- logs frame/tick counters, live instance count, render-object count, open file counters, and audio-stub call counters,
- records `dense_window_start`, `dense_window_end`, and first detected render frame markers,
- logs SDL lifecycle, low-memory, background, foreground, quit, and selected window events.

The idle crash is still treated as a D3 stability follow-up, not a failed D3 first-render proof. The physical D3 render proof remains complete.

User-observed marker from the visible diagnostics build:

```text
heartbeat=27 ticks=270315 stage=runtime-loop rss_bytes=90652672
```

The phone is configured not to auto-lock. The lifecycle events remain useful evidence, but normal auto-lock is no longer the lead theory. The next diagnostic pass focuses on what changes during the 240-300 second idle window, including menu/attract timers, event/timer behavior, gradual resource growth, and audio-stub retry patterns.

Latest pivot:

The idle-window log did not show an obvious memory or file-handle spike, but the audio stub counters rose steadily through the 240-300 second window. D3A therefore replaces the pure iOS audio stub with a minimal SDL2 audio backend:

- real SDL audio subsystem/device initialization,
- WAV effect loading through Bennu `file_open`,
- WAV conversion to the opened SDL device format,
- WAV playback through `SDL_QueueAudio`,
- stable inert handles for unsupported music/OGG paths,
- added audio counters in the same Files-visible heartbeat log.

This is still not full SDL2_mixer/OGG/Vorbis support. Music may remain silent. The test goal is stability and avoiding repeated broken/stubbed audio failures before D4a touch input.

Initial physical D3A result:

```text
audio_init_attempts=1
audio_init_ok=1
audio_init_fail=0
audio_wav_load_ok=122
audio_wav_load_fail=0
audio_wav_play=304
audio_stub_minus_one=0
heartbeat=49 ticks=274472 runtime_ms=273127
rss_bytes approximately 145 MB
```

The app still exited around five minutes, so the next diagnostic pass keeps real SFX audio but splits the remaining `audio_stub_zero` calls into named categories and logs BGM/music file-open attempts.

## Stability Log

The local-only iPhone log is:

```text
Library/Application Support/SORR/logs/ios_d3_runtime_stability_probe.txt
```

The same D3S log is mirrored to the app's Files-visible Documents area:

```text
On My iPhone/SorrIOSShell/SORR_DIAGNOSTICS/ios_d3_runtime_stability_probe.txt
```

Internal path:

```text
Documents/SORR_DIAGNOSTICS/ios_d3_runtime_stability_probe.txt
```

The app also writes:

```text
Documents/SORR_DIAGNOSTICS/README_D3S_DIAGNOSTICS.txt
```

This small local file exists only on the phone and makes the diagnostics folder easier to identify in Files.

Important markers:

- `stage=app-launch`
- `previous stability last marker=...`
- `D2 data preflight ready=...`
- `heartbeat timer started interval_ms=10000 dense_start_ms=240000 dense_end_ms=330000 dense_interval_ms=1000`
- `stage=runtime-loop`
- `runtime loop start ticks=... dense_start_ms=240000 dense_end_ms=330000`
- `first_frame_detected ticks=... runtime_ms=... frame_count=...`
- `dense_window_start ticks=... runtime_ms=... stage=runtime-loop`
- `heartbeat=N ticks=... runtime_ms=... interval_next_ms=... stage=runtime-loop rss_bytes=... frame_count=... instances=... render_objects=... opened_files=... audio_stub_zero=... audio_stub_minus_one=... audio_init_attempts=... audio_init_ok=... audio_init_fail=... audio_wav_load_ok=... audio_wav_load_fail=... audio_wav_play=... audio_inert_handles=... audio_queue_clears=... audio_music_load_attempts=... audio_music_open_ok=... audio_music_open_fail=... audio_live_handles=... audio_live_wav=... audio_live_inert_wav=... audio_live_music=... audio_max_live_handles=... audio_zero_music_play=... audio_zero_music_control=... audio_zero_music_query=... audio_zero_wav_control=... audio_zero_wav_query=... audio_zero_wav_volume=... audio_zero_channel_effect=... audio_zero_play_wav_guard=... audio_music_last_status=... audio_music_last_path=...`
- `dense_window_end ticks=... runtime_ms=... stage=runtime-loop`
- `event=SDL_APP_WILLENTERBACKGROUND`
- `event=SDL_APP_DIDENTERBACKGROUND`
- `event=SDL_APP_LOWMEMORY`
- `event=SDL_APP_TERMINATING`
- `instance_go_all returned ret=...`

If the app exits or crashes, reopen it once. The next launch should log and briefly report the previous stability log's last non-empty marker, making it easier to tell whether the app died while foregrounded in the runtime loop, while backgrounding, under low memory, or through a clean SDL quit path.

## Artifact Target

GitHub Actions device artifact:

```text
ios-shell-d3s-idle-window-device-arm64
```

IPA inside artifact:

```text
build-products/SorrIOSShell-d3s-idle-window-adhoc.ipa
```

The IPA remains asset-free. The workflow inspection still rejects `SorR.dat`, `data/`, `.fpg`, `.wav`, `.ogg`, `.smk`, and `.png` content.

Previous GitHub-side D3S visible diagnostics artifact proof:

```text
Actions run: 26552137370
Device artifact: ios-shell-d3s-visible-diagnostics-device-arm64
Artifact size: 675260 bytes
IPA: build-products/SorrIOSShell-d3s-visible-diagnostics-adhoc.ipa
Artifact-producing commit: 692c73b
Device job result: success
Game data/assets bundled in IPA: no
```

Current idle-window diagnostic target:

```text
Artifact: ios-shell-d3s-idle-window-device-arm64
IPA: build-products/SorrIOSShell-d3s-idle-window-adhoc.ipa
Focus: one-second diagnostics during runtime_ms=240000..330000
Game data/assets bundled in IPA: no
```

Current D3A audio/stability diagnostic target:

```text
Artifact: ios-shell-d3a-audio-diagnostics-device-arm64
IPA: build-products/SorrIOSShell-d3a-audio-diagnostics-adhoc.ipa
Focus: minimal real SDL audio backend, named audio no-op categories, BGM file-open diagnostics, and the existing 240000..330000 ms dense diagnostics
Game data/assets bundled in IPA: no
```

GitHub-side D3A follow-up diagnostic artifact proof:

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

GitHub-side D3A audio/stability artifact proof:

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

Initial D3A physical test result:

```text
SFX audio init/load/play counters: working
audio_stub_minus_one: no longer climbing
audio_stub_zero: still climbing
Foreground idle result: still exits around five minutes
Likely next evidence needed: named audio zero category and music/BGM file-open counters
```

GitHub-side D3S idle-window diagnostics artifact proof:

```text
Actions run: 26553120903
Device artifact: ios-shell-d3s-idle-window-device-arm64
Artifact size: 677186 bytes
IPA: build-products/SorrIOSShell-d3s-idle-window-adhoc.ipa
IPA size: 601806 bytes
Artifact-producing commit: f6b4347
Device job result: success
Game data/assets bundled in IPA: no
```

## Manual iPhone Test

1. Keep the D2-staged data on the iPhone.
2. If the staged data may be missing BGM, regenerate the local-only import package with `powershell -ExecutionPolicy Bypass -File tools\create_d2_import_package.ps1` and refresh the D2 import through Files. The helper now verifies `SORR_IMPORT/mod/music/1.ogg`.
3. Download `ios-shell-d3a-audio-diagnostics-device-arm64` from GitHub Actions.
4. Extract the artifact on Windows.
5. Install `build-products/SorrIOSShell-d3a-audio-diagnostics-adhoc.ipa` through Sideloadly.
6. Open `SorrIOSShell`.
7. Confirm the real SoRR render still appears.
8. Note whether any audio is audible.
9. Leave the app foregrounded and untouched until it exits or 10-15 minutes pass.
10. If it exits, reopen it once.
11. Open Files: `On My iPhone -> SorrIOSShell -> SORR_DIAGNOSTICS`.
12. Copy or screenshot the last 40-60 lines of `ios_d3_runtime_stability_probe.txt`.
13. If present, include the block from `dense_window_start` through `dense_window_end`, especially the `audio_zero_*` and `audio_music_last_*` fields.

## Latest D3A Audio/Music Triage

The memory-backed SDL2_mixer path now proves BGM and SFX are working on the physical iPhone:

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

The app still exits near the old 240-300 second window, so the next diagnostic target shifts from audio to timed runtime/game behavior. The follow-up D3S IPA keeps music enabled and logs interpreter/runtime snapshots in the Files-visible diagnostics:

```text
Artifact target: ios-shell-d3s-runtime-window-diagnostics-device-arm64
IPA target: build-products/SorrIOSShell-d3s-runtime-window-diagnostics-adhoc.ipa
Key new fields: runtime_loops, runtime_frames, runtime_runs, runtime_last_proc, runtime_snapshot
```

GitHub-side artifact proof:

```text
Actions run: 26592326534
Artifact: ios-shell-d3s-runtime-window-diagnostics-device-arm64
Artifact size: 796018 bytes
IPA: build-products/SorrIOSShell-d3s-runtime-window-diagnostics-adhoc.ipa
Artifact-producing commit: 7f11519
Device job result: success
Simulator job result: success
Game data/assets bundled in IPA: no
```
14. Do not start D4a controls until this pass is reviewed.

## Expected Results

Best case: the SDL audio device opens, WAV effects can be queued, unsupported music remains inert or silent, and the app keeps rendering for 10-15 minutes.

Diagnostic case: if the app still exits, the next launch's previous marker and dense-window heartbeat block should identify the last runtime stage, frame count, process/object/file counters, audio init/load/play/inert-handle counters, lifecycle event, low-memory event, termination event, or runtime return marker before the exit.
## D3S Attract Lifecycle Diagnostics

Latest physical runtime-window feedback shows the app is no longer in a passive menu near the five-minute exit. BGM and SFX are healthy, files are stable, and the runtime snapshots show active attract/demo/gameplay-like processes including `CONTROLADOR`, `BARRA_NEGRA`, `MELODIA`, `SOMBRA`, `ESTIRAMIENTO`, `BARRA_SEC_VIDA1`, `BARRA_VIDA1`, `EFECTO_POLVO`, `LETRA_NOMBRE`, and `MINI_CUADRO1`.

Next artifact target:

- `ios-shell-d3s-attract-lifecycle-diagnostics-device-arm64`
- `build-products/SorrIOSShell-d3s-attract-lifecycle-diagnostics-adhoc.ipa`

This build keeps real BGM/SFX enabled and adds process watch counts plus a recent create/destroy lifecycle ring to the Files-visible stability log. The goal is to identify whether a specific attract/demo transition, HUD/enemy/player process spike, invalid process status, or fatal signal happens after heartbeat 47-49.

When testing, report the last 80-120 lines from `Documents/SORR_DIAGNOSTICS/ios_d3_runtime_stability_probe.txt`, especially `runtime_snapshot`, `watch=`, `runtime_lifecycle`, direct `runtime_lifecycle ...` lines, and any `signal=` line.
## GitHub-Side D3S Attract Lifecycle Artifact Proof

Produced successfully:

- Actions run: `26595939671`
- Commit: `f0553b4`
- Artifact: `ios-shell-d3s-attract-lifecycle-diagnostics-device-arm64`
- IPA: `build-products/SorrIOSShell-d3s-attract-lifecycle-diagnostics-adhoc.ipa`
- Artifact size: `800173` bytes

This is the next on-device diagnostic build for the five-minute D3S exit. It keeps BGM and SFX enabled, does not bundle game data/assets, and should be tested with the existing D2-staged private data.
