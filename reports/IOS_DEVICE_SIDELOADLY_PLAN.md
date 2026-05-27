# iOS Device Sideloadly Plan

Date: 2026-05-27

## Goal

Produce a physical-device `iphoneos` `arm64` IPA from GitHub Actions that contains only `SorrIOSShell.app`.

The IPA is intended for Windows download and Sideloadly signing/install.

## Result

Status: D1 GitHub Actions ad-hoc-signed device IPA artifact proof complete, physical Sideloadly retest pending.

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

The same workflow run also kept the simulator shell/data-layout proof green. The GitHub-side D1 artifact has been rebuilt with an ad-hoc app signature; the remaining D1 check is the manual Sideloadly install/open test on the physical iPhone.

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
- [ ] User downloads the ad-hoc-signed IPA on Windows for Sideloadly signing/install.
- [ ] User confirms the installed app opens on the physical iPhone and reaches the shell idle loop.

Physical-device launch is the manual final check for D1 because CI does not have the user's iPhone.
