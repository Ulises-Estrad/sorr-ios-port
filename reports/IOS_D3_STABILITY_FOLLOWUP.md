# iOS D3 Stability Follow-Up

Status: visible diagnostics IPA produced by GitHub Actions.

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
- logs SDL lifecycle, low-memory, background, foreground, quit, and selected window events.

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
- `heartbeat timer started interval_ms=10000`
- `stage=runtime-loop`
- `heartbeat=N ticks=... stage=runtime-loop rss_bytes=...`
- `event=SDL_APP_WILLENTERBACKGROUND`
- `event=SDL_APP_DIDENTERBACKGROUND`
- `event=SDL_APP_LOWMEMORY`
- `event=SDL_APP_TERMINATING`
- `instance_go_all returned ret=...`

If the app exits or crashes, reopen it once. The next launch should log and briefly report the previous stability log's last non-empty marker, making it easier to tell whether the app died while foregrounded in the runtime loop, while backgrounding, under low memory, or through a clean SDL quit path.

## Artifact Target

GitHub Actions device artifact:

```text
ios-shell-d3s-visible-diagnostics-device-arm64
```

IPA inside artifact:

```text
build-products/SorrIOSShell-d3s-visible-diagnostics-adhoc.ipa
```

The IPA remains asset-free. The workflow inspection still rejects `SorR.dat`, `data/`, `.fpg`, `.wav`, `.ogg`, `.smk`, and `.png` content.

GitHub-side D3S visible diagnostics artifact proof:

```text
Actions run: 26552137370
Device artifact: ios-shell-d3s-visible-diagnostics-device-arm64
Artifact size: 675260 bytes
IPA: build-products/SorrIOSShell-d3s-visible-diagnostics-adhoc.ipa
Artifact-producing commit: 692c73b
Device job result: success
Game data/assets bundled in IPA: no
```

## Manual iPhone Test

1. Keep the D2-staged data on the iPhone.
2. Download `ios-shell-d3s-visible-diagnostics-device-arm64` from GitHub Actions.
3. Extract the artifact on Windows.
4. Install `build-products/SorrIOSShell-d3s-visible-diagnostics-adhoc.ipa` through Sideloadly.
5. Open `SorrIOSShell`.
6. Leave the app foregrounded and untouched until it exits or 10-15 minutes pass.
7. If it exits, reopen it once.
8. Open Files: `On My iPhone -> SorrIOSShell -> SORR_DIAGNOSTICS`.
9. Copy or screenshot the last 20 lines of `ios_d3_runtime_stability_probe.txt`.
10. Do not start D4a controls until this pass is reviewed.

## Expected Results

Best case: the idle timer fix prevents the five-minute idle exit and the app keeps rendering.

Diagnostic case: if the app still exits, the next launch's previous marker should identify the last heartbeat, lifecycle event, low-memory event, termination event, or runtime return marker before the exit.
