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

## D4a Fixed Touch Install

D4a uses the same Windows + Sideloadly install route and the same D2-staged private data. It adds fixed on-screen controls while keeping BGM/SFX and the D3S stability guards enabled.

Artifact target:

```text
ios-shell-d4a-fixed-touch-guarded-device-arm64
build-products/SorrIOSShell-d4a-fixed-touch-guarded-adhoc.ipa
```

Install/test:

1. Keep the existing `Library/Application Support/SORR` data from D2 on the iPhone.
2. Download and extract the D4a artifact on Windows.
3. Install `build-products/SorrIOSShell-d4a-fixed-touch-guarded-adhoc.ipa` with Sideloadly.
4. Launch `SorrIOSShell`.
5. Test fixed controls: D-pad, Attack, Jump, Special, Police, Start/Pause, and Back/Menu.
6. If it crashes, reopen once and retrieve:

```text
On My iPhone/SorrIOSShell/SORR_DIAGNOSTICS/ios_latest_crash_report.txt
```

Send `ios_current_run_stability_log.txt` only if more context is needed. The IPA must still contain no `SorR.dat`, `data/`, prepared data, or game assets.

D4a GitHub-side artifact proof:

```text
Actions run: 26607711467
Patch commit: dc64c13
Artifact: ios-shell-d4a-fixed-touch-guarded-device-arm64
IPA: build-products/SorrIOSShell-d4a-fixed-touch-guarded-adhoc.ipa
Artifact size: 811502 bytes
Device job result: success
Simulator job result: success
Game data/assets bundled in IPA: no
```

Use this artifact for the first fixed-touch physical iPhone test.

### D4a Visible Touch / Viewport Follow-Up

Physical testing of the first D4a artifact confirmed the fixed controls work, the game renders, and BGM/SFX work. The remaining issue is visual only: the controls are invisible, side pillar bars are white, and clipped overlay rectangles appear around the viewport.

Next artifact target:

```text
ios-shell-d4a-visible-touch-viewport-device-arm64
build-products/SorrIOSShell-d4a-visible-touch-viewport-adhoc.ipa
```

This follow-up keeps the working touch hitboxes and Bennu key injection unchanged. It forces the game-frame background clear to black and resets SDL logical size, viewport, clip rect, scale, and blend state before drawing the visible overlay in full drawable coordinates.

Install through the same Windows + Sideloadly route. Verify that the buttons are visible, the bars are black, the clipped top-right/bottom-middle artifacts are gone, and the controls still operate the game.

GitHub-side artifact proof:

```text
Actions run: 26608727747
Patch commit: ec24f1d
Artifact: ios-shell-d4a-visible-touch-viewport-device-arm64
IPA: build-products/SorrIOSShell-d4a-visible-touch-viewport-adhoc.ipa
Artifact size: 812321 bytes
Device job result: success
Simulator job result: success
Game data/assets bundled in IPA: no
```

### D4a Compact D-Pad Follow-Up

Physical testing confirmed the visible overlay and working gameplay controls, but the movement layout needs a traditional compact D-pad. The next artifact preserves the existing action buttons and changes only movement handling:

```text
ios-shell-d4a-dpad-refine-device-arm64
build-products/SorrIOSShell-d4a-dpad-refine-adhoc.ipa
```

Install through the same Windows + Sideloadly route. Test sliding Right to Left, Right to Up, and Up to Down without lifting. The previous direction should release unless the touch is intentionally in a diagonal zone. Also confirm direction plus Attack/Jump multitouch still works.

GitHub-side artifact proof:

```text
Actions run: 26611068603
Patch commit: 95ae6d2
Artifact: ios-shell-d4a-dpad-refine-device-arm64
IPA: build-products/SorrIOSShell-d4a-dpad-refine-adhoc.ipa
Artifact size: 813146 bytes
Device job result: success
Simulator job result: success
Game data/assets bundled in IPA: no
Physical iPhone result: pending manual D-pad slide test
```

### D4b Custom Touch Follow-Up

Physical testing of the compact D-pad artifact showed that movement can still feel locked to the first held direction. The next artifact combines the D4a movement-feel fix with D4b-lite customization:

```text
ios-shell-d4b-custom-touch-device-arm64
build-products/SorrIOSShell-d4b-custom-touch-adhoc.ipa
```

Install through the same Windows + Sideloadly route. The private D2-staged data remains on the phone; the IPA still bundles no game data.

Test focus:

1. Slide Right to Left without lifting: Right should release and Left should press.
2. Slide Right to Up: Right should release unless the touch is intentionally in a diagonal zone.
3. Hold D-pad direction plus Attack/Jump/Special.
4. Tap `CFG`, drag controls, use `BIG`/`SML`, `OPAC`, `HIDE`/`SHOW`, `RST`, and save with `DONE`.
5. Relaunch and confirm the layout persists from `On My iPhone/SorrIOSShell/SORR_DIAGNOSTICS/ios_touch_controls.ini`.

GitHub-side artifact proof:

```text
Actions run: 26612339231
Patch commit: f5beb9a
Artifact: ios-shell-d4b-custom-touch-device-arm64
IPA: build-products/SorrIOSShell-d4b-custom-touch-adhoc.ipa
Artifact size: 798 KB
Device job result: success
Simulator job result: success
Game data/assets bundled in IPA: no
Physical iPhone result: pending manual D-pad/customization test
```

### Current Consolidated Baseline

D4b custom touch is the current usable baseline. The next device artifact adds the temporary app icon and keeps the same Sideloadly install path:

```text
ios-shell-d4b-custom-touch-icon-device-arm64
build-products/SorrIOSShell-d4b-custom-touch-icon-adhoc.ipa
```

GitHub-side consolidated icon artifact proof:

```text
Actions run: 26613847549
Commit: 444d0a0
Artifact: ios-shell-d4b-custom-touch-icon-device-arm64
IPA: build-products/SorrIOSShell-d4b-custom-touch-icon-adhoc.ipa
Artifact size: 1955535 bytes
Device job result: success
Simulator job result: success
Game data/assets bundled in IPA: no
```

## D4b Joystick Control Artifact

After physical testing, the app icon is accepted and the current remaining control issue is movement feel: the compact D-pad can still feel locked or stiff. The next device IPA keeps the existing action buttons, custom-control persistence, BGM/SFX, D3S guards, and asset-free package, but swaps the movement UI/behavior to a virtual joystick that maps to the same Bennu arrow keys.

Artifact target:

```text
ios-shell-d4b-joystick-controls-icon-device-arm64
```

IPA target:

```text
build-products/SorrIOSShell-d4b-joystick-controls-icon-adhoc.ipa
```

Manual test focus:

1. Install the IPA with Sideloadly over the previous build.
2. Confirm the app icon remains correct.
3. Confirm `CFG` responds consistently and edit mode still saves settings.
4. Confirm joystick movement is fluid: Right to Left and Up to Down should update without lifting.
5. Confirm joystick plus Attack/Jump/Special/Police works.
6. Confirm BGM/SFX and crash-report files remain available.

GitHub-side D4b joystick-control icon artifact proof:

```text
Actions run: 26617138664
Commit: 64dee98
Artifact: ios-shell-d4b-joystick-controls-icon-device-arm64
IPA: build-products/SorrIOSShell-d4b-joystick-controls-icon-adhoc.ipa
Artifact size: 1956172 bytes
Device job result: success
Simulator job result: success
Game data/assets bundled in IPA: no
Physical iPhone result: pending manual joystick/CFG test
```

## D4b CFG / Joystick Config Fix Artifact

Physical testing found that the enlarged CFG visual did not match reliable tap behavior and DONE could fail similarly. The next Sideloadly IPA keeps the joystick and working gameplay controls, but fixes the config UI:

- suppress duplicate synthetic mouse events after iOS touch events,
- place the edit toolbar vertically on the left instead of across Start/Back,
- replace `HIDE` / `SHOW` with `TXT+` / `TXT-` for gameplay button labels,
- hide gameplay button text by default while preserving labels in edit mode,
- keep BGM/SFX, D3S guards, crash reporting, app icon, and asset-free packaging.

Artifact target:

```text
ios-shell-d4b-config-joystick-icon-device-arm64
```

IPA target:

```text
build-products/SorrIOSShell-d4b-config-joystick-icon-adhoc.ipa
```

Manual test focus:

1. Tap `CFG` repeatedly and confirm it opens edit mode consistently.
2. Tap `DONE` repeatedly and confirm it exits edit mode consistently.
3. Confirm Start/Back are not covered by the edit toolbar.
4. Confirm `TXT+` / `TXT-` toggles gameplay text labels.
5. Confirm joystick/action gameplay controls still work.

GitHub-side D4b CFG/joystick config artifact proof:

```text
Actions run: 26618164911
Commit: b587ee3
Artifact: ios-shell-d4b-config-joystick-icon-device-arm64
IPA: build-products/SorrIOSShell-d4b-config-joystick-icon-adhoc.ipa
Artifact size: 1956085 bytes
Device job result: success
Simulator job result: success
Game data/assets bundled in IPA: no
Physical iPhone result: pending manual CFG/DONE/toolbar/text-label test
```

## D4b Joystick Release / Pillar CFG Artifact

Physical testing found that CFG is better but not fully clean, and the joystick can stay stuck in the last direction until CFG resets the control state. The next Sideloadly IPA keeps the same control scheme but fixes release handling and moves CFG into a smaller pillar-safe position.

Artifact target:

```text
ios-shell-d4b-joystick-release-cfg-device-arm64
```

IPA target:

```text
build-products/SorrIOSShell-d4b-joystick-release-cfg-adhoc.ipa
```

Manual test focus:

1. Press and release each joystick direction; no direction should remain stuck.
2. Hold a direction for more than a second, release, and confirm movement stops.
3. Confirm the small opaque CFG button sits in the left pillar area and is not clipped by the rounded screen corner.
4. Confirm CFG opens and DONE exits.
5. Confirm Start/Back and action buttons are still usable.

GitHub-side D4b joystick-release / pillar CFG artifact proof:

```text
Actions run: 26618833984
Commit: e4cbf2b
Artifact: ios-shell-d4b-joystick-release-cfg-device-arm64
IPA: build-products/SorrIOSShell-d4b-joystick-release-cfg-adhoc.ipa
Artifact size: 1956468 bytes
Device job result: success
Simulator job result: success
Game data/assets bundled in IPA: no
App display name: Streets of Rage
Physical iPhone result: pending manual joystick-release/CFG placement test
```

## Current Playtest Controls Baseline

This baseline is no longer just a proof-only shell. Real SoRR runs on iPhone, BGM/SFX work, virtual joystick and action controls work, custom controls persist, and crash reporting remains available. Remaining work is playtesting, control tuning, polish, and major bug fixes found during real play.

Current Sideloadly artifact target:

```text
ios-shell-playtest-controls-device-arm64
```

Current IPA target:

```text
build-products/StreetsOfRage-playtest-controls-adhoc.ipa
```

Manual test focus:

1. Install the current IPA with Sideloadly.
2. Confirm `Streets of Rage` launches with the existing staged data.
3. Confirm joystick movement releases correctly.
4. Confirm `CFG` sits in the left pillar-safe area.
5. Confirm `START` sits in the right pillar-safe area and `BACK` sits below it.
6. Confirm edit-mode layout changes require dragging, not just tapping.
7. Confirm BGM/SFX and crash-report files remain available.

For a full recovery path from zero, use the root `INSTALL_IOS.md` guide.
