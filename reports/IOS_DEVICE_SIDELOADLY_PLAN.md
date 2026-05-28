# iOS Device Sideloadly Plan

Date: 2026-05-27

## Goal

Produce a physical-device `iphoneos` `arm64` IPA from GitHub Actions that contains only `SorrIOSShell.app`.

The IPA is intended for Windows download and Sideloadly signing/install.

## Result

Status: D1 physical iPhone Sideloadly install proof complete.

Proof run:

```text
Commit: 3909ccc
Run: https://github.com/Ulises-Estrad/sorr-ios-port/actions/runs/26537919700
Device job: Build iOS shell unsigned IPA for device arm64
Device job result: success
Artifact: ios-shell-device-unsigned-arm64
Artifact size: 445153 bytes
```

The same workflow run also kept the simulator shell/data-layout proof green.

First physical install result:

```text
Sideloadly v0.60
Install failed: Guru Meditation f65043@1006:23a71c Invalid file
IPA: SorrIOSShell-device-unsigned.ipa
```

Interpretation:

- USB device detection is fixed.
- GitHub Actions produced an IPA artifact.
- Sideloadly rejected the IPA before app launch.
- The first likely issue is a completely unsigned `iphoneos` app bundle inside the IPA.

Fix applied:

- Ad-hoc sign `SorrIOSShell.app` in CI before packaging.
- Rename the output to `SorrIOSShell-device-adhoc.ipa`.
- Inspect the IPA after packaging by unzipping it into a fresh directory.
- Verify `Info.plist`, `CFBundleExecutable`, `CFBundlePackageType`, `CFBundleIdentifier`, `MinimumOSVersion`, `UIDeviceFamily`, `arm64` Mach-O, and code signature.

Ad-hoc-signed proof run:

```text
Commit: 236455a
Run: https://github.com/Ulises-Estrad/sorr-ios-port/actions/runs/26540409093
Device job: Build iOS shell unsigned IPA for device arm64
Device job result: success
Artifact: ios-shell-device-unsigned-arm64
Artifact size: 456283 bytes
IPA inside artifact: build-products/SorrIOSShell-device-adhoc.ipa
```

The same workflow run also kept the simulator shell/data-layout proof green. The GitHub-side D1 artifact was rebuilt with an ad-hoc app signature.

Physical iPhone proof:

```text
Physical install route: Windows + Sideloadly
IPA: build-products/SorrIOSShell-device-adhoc.ipa
Result: installed after enabling Developer Mode and trusting the developer profile
iPhone behavior: SorrIOSShell opens to a dark/blank shell-only idle screen and remains open
Game data/assets: not bundled
```

The dark/blank screen is expected for D1 because this artifact intentionally contains only the iOS shell. It does not bundle `SorR.dat`, game data, assets, touch controls, or the game render path.

## Guardrails

This phase does not:

- bundle `SorR.dat`,
- bundle `data/`,
- bundle extracted game assets,
- load the game,
- start touch controls,
- start audio or SDL2_mixer,
- use App Store, TestFlight, or device provisioning in CI,
- require a paid Apple Developer account,
- touch known-good desktop or x64 snapshots.

## Artifact

Expected GitHub Actions artifact:

```text
ios-shell-device-unsigned-arm64
```

Expected IPA inside the artifact:

```text
build-products/SorrIOSShell-device-adhoc.ipa
```

Expected IPA layout:

```text
Payload/SorrIOSShell.app
```

## Workflow Shape

The device job is separate from the simulator launch-proof job:

```text
Build iOS shell unsigned IPA for device arm64
```

It builds:

- SDL2 2.30.12 for `iphoneos`,
- `SorrIOSShell.app` for `iphoneos` `arm64`,
- app bundle built with `CODE_SIGNING_ALLOWED=NO`,
- CI ad-hoc signature applied with `codesign --sign -` before packaging,
- IPA zip with `Payload/SorrIOSShell.app`.

The job does not install or run on a device.

## IPA Inspection

The workflow runs:

```bash
unzip -l SorrIOSShell-device-adhoc.ipa
```

Then it fails if the IPA contains any of:

```text
SorR.dat
data/
*.fpg
*.wav
*.ogg
*.smk
*.png
```

This keeps the CI artifact shell-only.

## Sideloadly Use

After the GitHub Actions job passes:

1. Download the `ios-shell-device-unsigned-arm64` artifact on Windows.
2. Extract `SorrIOSShell-device-adhoc.ipa`.
3. Open Sideloadly.
4. Select the IPA.
5. Connect the iPhone.
6. Let Sideloadly sign/install the IPA using the user's Apple ID.
7. Launch `SorrIOSShell` on the iPhone.

Expected app behavior:

- app opens,
- SDL shell starts,
- writable data scaffold is created inside the app container,
- shell reaches idle loop,
- no game data is loaded.

## Definition Of Done

- [x] GitHub Actions builds an `iphoneos` `arm64` `SorrIOSShell.app`.
- [x] GitHub Actions packages `Payload/SorrIOSShell.app` into `SorrIOSShell-device-unsigned.ipa`.
- [x] IPA artifact uploads successfully.
- [x] IPA inspection confirms no game data/assets.
- [x] User downloads the first IPA on Windows for Sideloadly signing/install.
- [x] GitHub Actions packages an ad-hoc-signed `SorrIOSShell-device-adhoc.ipa`.
- [x] User downloads the ad-hoc-signed IPA on Windows for Sideloadly signing/install.
- [x] User confirms the installed app opens on the physical iPhone and reaches the shell-only dark idle screen.

Physical-device launch is complete. D1 stops here; D2 must be started explicitly before any game data, touch controls, audio, or render-path work begins.

## D2 Successor Note

D2 is separate from this completed D1 proof. The D2 artifact is expected to be:

```text
ios-shell-d2-sorr-import-device-arm64
build-products/SorrIOSShell-d2-sorr-import-adhoc.ipa
```

It still must not bundle `SorR.dat`, `data/`, or game assets. D2 uses iOS file sharing so prepared data can be copied privately through the Files app into `Documents/SORR_IMPORT` after Sideloadly installation. The app treats that folder as an inbox and stages valid direct or one-folder-nested data into `Library/Application Support/SORR`.

Practical Windows-to-iPhone D2 route:

1. On Windows, create `SORR_IMPORT` with `SorR.dat`, `mod/system.txt`, `savegame`, `xbox`, and the remaining prepared data.
2. Transfer the folder or a zip of it through iCloud Drive, iCloud.com, OneDrive, Google Drive, or another iOS Files-visible provider.
3. On the iPhone, extract the zip if needed.
4. Copy either the data contents or the one extracted top-level folder into `On My iPhone` -> `SorrIOSShell` -> `SORR_IMPORT`.
5. Relaunch the app and verify the D2 status screen reports a direct or one-folder nested layout, staged data, found/opened required files, and writable probe paths.

Previous D2 GitHub-side artifact proof:

```text
Commit: 5d9036f
Run: https://github.com/Ulises-Estrad/sorr-ios-port/actions/runs/26541978819
Artifact: ios-shell-d2-data-import-device-arm64
IPA: build-products/SorrIOSShell-d2-data-import-adhoc.ipa
Result: success
```

Updated D2 target artifact:

```text
Artifact: ios-shell-d2-sorr-import-device-arm64
IPA: build-products/SorrIOSShell-d2-sorr-import-adhoc.ipa
```

Updated D2 GitHub-side artifact proof:

```text
Commit: 7b395b3
Run: https://github.com/Ulises-Estrad/sorr-ios-port/actions/runs/26543157389
Artifact: ios-shell-d2-sorr-import-device-arm64
Artifact size: 461067 bytes
IPA: build-products/SorrIOSShell-d2-sorr-import-adhoc.ipa
Result: success
```

D2 physical iPhone proof:

```text
Passing Actions run: 26543157389
Artifact: ios-shell-d2-sorr-import-device-arm64
IPA: build-products/SorrIOSShell-d2-sorr-import-adhoc.ipa
Install route: Windows + Sideloadly
Local-only import package: out/local-only/SORR_IMPORT.zip
Local package source: sorr-vita-master/data
Local package SHA256: B2E2F3901C65BE0FA3D739D3716C74C37AE13FB9BDF7535ADC487D9A3D22C72A
Physical result: direct layout detected, staging copied, SorR.dat opened, mod/system.txt found, savegame/xbox/logs writable, probe log OK
Game execution: no
Game rendering: no
Game data/assets committed or bundled in IPA: no
```

D2 is complete and D3 has since been explicitly requested and completed.

## D3 Successor Note

D3 is the physical iPhone first render proof. It reuses the D2-staged data at `Library/Application Support/SORR`; the IPA still does not bundle or commit private game data.

```text
D3 Actions run: 26549967383
Artifact: ios-shell-d3-first-render-device-arm64
IPA: build-products/SorrIOSShell-d3-first-render-adhoc.ipa
Artifact-producing commit: d79a53a
D3 docs commit before physical proof: 148ead7 [skip ci]
Install route: Windows + Sideloadly
Physical result: actual SoRR game/runtime rendered and the game ran on the physical iPhone
Audio result: no audible audio yet
Game data/assets bundled in IPA: no
Game assets committed: no
D4/D5 status: not started
```

The missing audio is a later follow-up, not a D3 failure. Do not proceed to D4 controls, D5 gameplay, or audio follow-up work until explicitly instructed.

## D3 Stability Follow-Up

D3 first render remains complete, but a repeatable idle exit was observed after leaving the physical iPhone app untouched for about five minutes. A D3 stability artifact is now prepared before D4a controls:

```text
ios-shell-d3-stability-device-arm64
build-products/SorrIOSShell-d3-stability-adhoc.ipa
```

GitHub-side artifact proof:

```text
Actions run: 26551315103
Artifact: ios-shell-d3-stability-device-arm64
Artifact size: 675392 bytes
IPA: build-products/SorrIOSShell-d3-stability-adhoc.ipa
Artifact-producing commit: a0a401d
Device job result: success
Game data/assets bundled in IPA: no
```

Install through the same Windows + Sideloadly route as D1-D3. Keep the D2-staged data on the device. Launch the app, leave it foregrounded and untouched for at least 7 minutes, and if it exits, reopen once and report the previous stability marker.

The stability build still bundles no game data and writes its diagnostic log locally on the iPhone:

```text
Library/Application Support/SORR/logs/ios_d3_runtime_stability_probe.txt
```

D3S visible diagnostics update:

The first stability IPA still exited after about five minutes, and the app-private log was not visible through Files. The next D3S artifact mirrors the same log to the app's visible Documents area:

```text
On My iPhone/SorrIOSShell/SORR_DIAGNOSTICS/ios_d3_runtime_stability_probe.txt
```

Artifact target:

```text
ios-shell-d3s-visible-diagnostics-device-arm64
build-products/SorrIOSShell-d3s-visible-diagnostics-adhoc.ipa
```

GitHub-side artifact proof:

```text
Actions run: 26552137370
Artifact: ios-shell-d3s-visible-diagnostics-device-arm64
Artifact size: 675260 bytes
IPA: build-products/SorrIOSShell-d3s-visible-diagnostics-adhoc.ipa
Artifact-producing commit: 692c73b
Device job result: success
Game data/assets bundled in IPA: no
```

After an idle exit, reopen the app once, then open Files and retrieve the last 20 lines from `SORR_DIAGNOSTICS/ios_d3_runtime_stability_probe.txt`.

D3S idle-window diagnostics update:

The visible diagnostics build still exited after about five foreground idle minutes. The iPhone is configured not to auto-lock, so this pass focuses on the runtime window around 240-330 seconds rather than assuming normal lock/sleep behavior.

Artifact target:

```text
ios-shell-d3s-idle-window-device-arm64
build-products/SorrIOSShell-d3s-idle-window-adhoc.ipa
```

This IPA keeps the same D2 data and D3 render path, but logs one-second heartbeats during `runtime_ms=240000..330000` with frame counters, live instance count, render-object count, file counters, and audio-stub call counters.

Install through the same Windows + Sideloadly route. Leave the app foregrounded and untouched for 10-15 minutes. If it exits, reopen it once, then retrieve the last 40-60 lines from:

```text
On My iPhone/SorrIOSShell/SORR_DIAGNOSTICS/ios_d3_runtime_stability_probe.txt
```

If present, include the block from `dense_window_start` through `dense_window_end`.

GitHub-side D3S idle-window artifact proof:

```text
Actions run: 26553120903
Artifact: ios-shell-d3s-idle-window-device-arm64
Artifact size: 677186 bytes
IPA: build-products/SorrIOSShell-d3s-idle-window-adhoc.ipa
IPA size: 601806 bytes
Artifact-producing commit: f6b4347
Device job result: success
Game data/assets bundled in IPA: no
```

D3A audio/stability update:

The idle-window diagnostics still showed a repeatable foreground exit near five minutes. Memory and file counters were not the obvious cause, while audio stub counters rose steadily during the 240-300 second window. The first D3A artifact restored a minimal SDL2 audio backend for iOS:

- opens the real SDL audio device,
- loads WAV effects through Bennu `file_open`,
- queues WAV effects with `SDL_QueueAudio`,
- returns stable inert handles for unsupported music/OGG paths,
- keeps the same Files-visible diagnostics path.

Artifact target:

```text
ios-shell-d3a-audio-diagnostics-device-arm64
build-products/SorrIOSShell-d3a-audio-diagnostics-adhoc.ipa
```

The follow-up diagnostic artifact keeps that SFX audio path and adds named audio no-op categories, music/BGM file-open diagnostics, and live audio handle counters. Install through the same Windows + Sideloadly route. Leave the app foregrounded and untouched for 10-15 minutes. If it exits, reopen it once, then retrieve the last 40-60 lines from:

```text
On My iPhone/SorrIOSShell/SORR_DIAGNOSTICS/ios_d3_runtime_stability_probe.txt
```

Audio may be partial: WAV effects may be audible; music may remain silent until SDL2_mixer/OGG/Vorbis is added in a later explicit audio milestone.

GitHub-side D3A audio/stability artifact proof:

```text
Actions run: 26554585872
Artifact: ios-shell-d3a-audio-device-arm64
Artifact size: 724145 bytes
IPA: build-products/SorrIOSShell-d3a-audio-adhoc.ipa
Artifact-producing commit: d6785c7
Device job result: success
Simulator job result: success
Game data/assets bundled in IPA: no
Physical idle result: initial D3A audio pass still exited around five minutes
SFX result: audio_init_ok=1, audio_wav_load_ok=122, audio_wav_play=304
Next artifact: ios-shell-d3a-audio-diagnostics-device-arm64
```

GitHub-side D3A follow-up diagnostic artifact proof:

```text
Actions run: 26557322892
Artifact: ios-shell-d3a-audio-diagnostics-device-arm64
Artifact size: 728042 bytes
IPA: build-products/SorrIOSShell-d3a-audio-diagnostics-adhoc.ipa
Artifact-producing commit: 0d3eb53
Device job result: success
Simulator job result: success
Game data/assets bundled in IPA: no
```
