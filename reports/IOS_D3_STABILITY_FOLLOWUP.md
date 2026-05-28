# iOS D3 Stability Follow-Up

Status: idle-window diagnostics in progress.

D3 first render remains complete. This pass hardens the already-rendering D3 build after a repeatable physical-device idle exit was observed at about five minutes.

## Scope

This is D3 stability only.

No touch controls, gameplay input, audio polish, SOR2-only pruning, App Store/TestFlight signing, or bundled game data are part of this pass.

## Change Summary

The D3 iOS build now:

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
- `heartbeat=N ticks=... runtime_ms=... interval_next_ms=... stage=runtime-loop rss_bytes=... frame_count=... instances=... render_objects=... opened_files=... audio_stub_zero=... audio_stub_minus_one=...`
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

## Manual iPhone Test

1. Keep the D2-staged data on the iPhone.
2. Download `ios-shell-d3s-idle-window-device-arm64` from GitHub Actions.
3. Extract the artifact on Windows.
4. Install `build-products/SorrIOSShell-d3s-idle-window-adhoc.ipa` through Sideloadly.
5. Open `SorrIOSShell`.
6. Leave the app foregrounded and untouched until it exits or 10-15 minutes pass.
7. If it exits, reopen it once.
8. Open Files: `On My iPhone -> SorrIOSShell -> SORR_DIAGNOSTICS`.
9. Copy or screenshot the last 40-60 lines of `ios_d3_runtime_stability_probe.txt`.
10. If present, include the block from `dense_window_start` through `dense_window_end`.
11. Do not start D4a controls until this pass is reviewed.

## Expected Results

Best case: the idle timer fix prevents the five-minute idle exit and the app keeps rendering.

Diagnostic case: if the app still exits, the next launch's previous marker and dense-window heartbeat block should identify the last runtime stage, frame count, process/object/file/audio-stub counters, lifecycle event, low-memory event, termination event, or runtime return marker before the exit.
