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
