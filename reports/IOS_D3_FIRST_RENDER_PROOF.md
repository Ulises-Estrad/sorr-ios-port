# iOS D3 First Render Proof

Date: 2026-05-27

## Goal

D3 is the first physical iPhone render proof.

It uses the D2-staged data already stored at:

```text
Library/Application Support/SORR
```

D3 does not bundle `SorR.dat`, `data/`, extracted assets, or prepared game data into the IPA.

## Prior Proofs

D1 physical shell install is complete:

```text
Commit: dc13aaf50bc31afe9945c6b9410178642ed69e55
Result: shell-only IPA installed through Sideloadly and opened to the expected dark idle screen
```

D2 physical data import/storage is complete:

```text
Completion commit: 6af6e64
Passing Actions run: 26543157389
Artifact: ios-shell-d2-sorr-import-device-arm64
IPA: build-products/SorrIOSShell-d2-sorr-import-adhoc.ipa
Local-only import package: out/local-only/SORR_IMPORT.zip
ZIP SHA256: B2E2F3901C65BE0FA3D739D3716C74C37AE13FB9BDF7535ADC487D9A3D22C72A
Physical result: SorR.dat opened, mod/system.txt found, savegame/xbox/logs writable, probe log OK
Game execution: no
Game rendering: no
```

## D3 Build

The D3 device job enables the iOS runtime-linked probe:

```text
-DSORR_IOS_D3_FIRST_RENDER=ON
```

Expected GitHub Actions artifact:

```text
ios-shell-d3-first-render-device-arm64
```

Expected IPA inside the artifact:

```text
build-products/SorrIOSShell-d3-first-render-adhoc.ipa
```

## CI Iterations

First D3 attempt:

```text
Commit: 8a44a5b
Run: 26545508249
Result: device configure failed
Failure: vendored tre CMakeLists.txt declared cmake_minimum_required(VERSION 3.4), which CMake 4 rejects
Fix: set CMAKE_POLICY_VERSION_MINIMUM=3.5 in the iOS D3 parent CMake before add_subdirectory(tre)
```

Second D3 attempt:

```text
Commit: 1165d12
Run: 26545953893
Result: device compile failed
Failure: strings.c included windows.h after the diagnostic guard was widened for SORR_HOST_POINTER_TABLES
Fix: keep the Windows VirtualQuery readability probe under _WIN64 and use a non-Windows non-null diagnostic fallback for iOS
```

Third D3 attempt:

```text
Commit: 888c629
Run: 26546307412
Result: device compile failed
Failure: mod_sys.c includes UIKit Objective-C code for TARGET_IOS but was compiled as C
Fix: enable Objective-C for the D3 iOS project and mark only mod_sys.c as OBJC
```

The IPA must still contain no:

- `SorR.dat`
- `data/`
- `.fpg`
- `.wav`
- `.ogg`
- `.smk`
- `.png`
- prepared game data

## App Behavior

On launch, D3 keeps the D2 data preflight:

- resolve `Library/Application Support/SORR`,
- verify `SorR.dat`,
- verify `mod/system.txt`,
- verify writable `savegame`, `xbox`, and `logs`,
- write `logs/ios_d3_first_render_probe.txt`.

If D2 data is missing, the app stays on a visible status screen that tells the user to complete D2 first.

If D2 data is present, the app:

- switches the working directory to `Library/Application Support/SORR`,
- adds that folder to the Bennu file search path,
- sets `OS_ID=0` so the imported PC-prepared data keeps the desktop code path,
- keeps audio disabled/stubbed for D3,
- loads `SorR.dat`,
- initializes sysprocs and the runtime entry,
- hands control to the first script/render loop.

The title/menu/city scene are all acceptable first-render proof targets.

## Manual iPhone Test

1. Keep the D2 data already staged on the iPhone.
2. Download the `ios-shell-d3-first-render-device-arm64` artifact from GitHub Actions.
3. Extract the artifact on Windows.
4. Install `build-products/SorrIOSShell-d3-first-render-adhoc.ipa` through Sideloadly.
5. Open `SorrIOSShell` on the iPhone.
6. If the D2 staged data is present, expect the app to briefly show a D3 status screen and then attempt real SoRR rendering.
7. If the D2 staged data is missing, expect the app to show a D3/D2 missing-data status screen instead of a silent black screen.

## Logs

D3 writes a local-only probe log on the iPhone:

```text
Library/Application Support/SORR/logs/ios_d3_first_render_probe.txt
```

Useful markers:

- app support path
- `SorR.dat` open result
- runtime init start/end
- `dcb_load` result
- `sysproc_init` result
- first script execution start
- render-loop handoff begin

## Out Of Scope

- touch controls,
- gameplay input,
- audio/SDL2_mixer,
- App Store/TestFlight signing,
- SOR2-only pruning,
- Options submenu parity.
