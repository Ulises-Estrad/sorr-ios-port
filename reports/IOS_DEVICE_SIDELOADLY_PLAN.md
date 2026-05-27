# iOS Device Sideloadly Plan

Date: 2026-05-27

## Goal

Produce a physical-device `iphoneos` `arm64` IPA from GitHub Actions that contains only `SorrIOSShell.app`.

The IPA is intended for Windows download and Sideloadly signing/install.

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
build-products/SorrIOSShell-device-unsigned.ipa
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
- unsigned or unsigned-like app bundle with `CODE_SIGNING_ALLOWED=NO`,
- IPA zip with `Payload/SorrIOSShell.app`.

The job does not install or run on a device.

## IPA Inspection

The workflow runs:

```bash
unzip -l SorrIOSShell-device-unsigned.ipa
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
2. Extract `SorrIOSShell-device-unsigned.ipa`.
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

- GitHub Actions builds an `iphoneos` `arm64` `SorrIOSShell.app`.
- GitHub Actions packages `Payload/SorrIOSShell.app` into `SorrIOSShell-device-unsigned.ipa`.
- IPA artifact uploads successfully.
- IPA inspection confirms no game data/assets.
- User can download the IPA on Windows for Sideloadly signing/install.

Physical-device launch is the manual final check for D1 because CI does not have the user's iPhone.
