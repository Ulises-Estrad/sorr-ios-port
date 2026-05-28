# iOS D3 First Render Proof

Date: 2026-05-27

## Goal

D3 is the first physical iPhone render proof.

Status: D3 physical iPhone first render proof complete.

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

GitHub-side D3 artifact proof:

```text
Commit: d79a53a
Run: https://github.com/Ulises-Estrad/sorr-ios-port/actions/runs/26549967383
Artifact: ios-shell-d3-first-render-device-arm64
Artifact size: 674040 bytes
IPA inside artifact: build-products/SorrIOSShell-d3-first-render-adhoc.ipa
IPA size: 598798 bytes
Result: success
Asset status: no SorR.dat, data/, FPG/WAV/OGG/SMK/PNG assets, or prepared game data in the IPA
Physical iPhone result: real SoRR/game runtime rendered and ran from D2-staged data
Audio result: no audible audio yet; this is a known follow-up and not a D3 failure
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

Fourth D3 attempt:

```text
Commit: be532ad
Run: 26546610123
Result: device compile failed
Failure: mod_m7.c used int callback parameters where the render object API expects void * callback context
Fix: convert the mode7 render-object callback boundary to void * plus intptr_t casts
```

Fifth D3 attempt:

```text
Commit: 362fcb6
Run: 26547029368
Result: device compile failed
Failure: mod_flic.c used FLIC * callback parameters where the render object API expects void * callback context
Fix: convert the FLIC render-object callback boundary to void * and cast back to FLIC * inside the callbacks
```

Sixth D3 attempt:

```text
Commit: 1ce7c29
Run: 26547306387
Result: device compile failed
Failure: mod_draw.c could not include libdraw.h because the D3 iOS target missed the libdraw include path
Fix: add modules/libdraw to the D3 iOS target include directories
```

Seventh D3 attempt:

```text
Commit: 766ed3d
Run: 26547583863
Result: device compile failed
Failure: mod_draw.c used DRAWING_OBJECT * callback parameters where the render object API expects void * callback context
Fix: convert the mod_draw render-object callbacks to void * and cast back to DRAWING_OBJECT * inside the callbacks
```

Eighth D3 attempt:

```text
Commit: 90596ec
Run: 26547897400
Result: device compile failed
Failure: libtext.c used TEXT * callback parameters where the render object API expects void * callback context
Fix: convert the text render-object callbacks to void * and cast back to TEXT * inside the callbacks
```

Ninth D3 attempt:

```text
Commit: 8f80318
Run: 26548169964
Result: device compile failed
Failure: libscroll.c used int scroll-index callback parameters where the render object API expects void * callback context
Fix: convert the scroll render-object callbacks to void * plus intptr_t casts
```

Tenth D3 attempt:

```text
Commit: 8e5cfa1
Run: 26548410341
Result: device compile failed
Failure: libmouse.c used INSTANCE * callback parameters where the render object API expects void * callback context
Fix: convert mouse callbacks to void *; also convert the remaining included g_instance render-object callbacks found by local gr_new_object scan
```

Eleventh D3 attempt:

```text
Commit: 5597154
Run: 26548706584
Result: device compile failed
Failure: interpreter.c included windows.h on iOS after host pointer-table diagnostics were generalized for SORR_HOST_POINTER_TABLES
Fix: keep Windows VirtualQuery diagnostics under _WIN64; use a non-Windows non-null pointer diagnostic fallback for SORR_HOST_POINTER_TABLES
```

Twelfth D3 attempt:

```text
Commit: 1390351
Run: 26549118417
Result: device compile failed
Failure: interpreter.c still used Windows GetSystemMetrics in the host pointer-table GET_DESKTOP_SIZE bridge, and the diagnostic block closed one preprocessor guard too early
Fix: use a small non-Windows desktop-size fallback for SORR_HOST_POINTER_TABLES and keep the diagnostic helper functions inside the outer portable diagnostic guard
```

Thirteenth D3 attempt:

```text
Commit: 4aa5f00
Run: 26549458571
Result: device compile failed
Failure: g_blit.c passed a typed VERTEX comparator to qsort, which AppleClang rejects against the standard const void * comparator ABI
Fix: update compare_vertex_y to use the standard qsort comparator signature and cast to VERTEX internally
```

Fourteenth D3 attempt:

```text
Commit: 1468e75
Run: 26549773741
Result: device compile failed
Failure: dirs.c used GLOB_PERIOD, which is not provided by the iPhoneOS glob headers
Fix: define GLOB_PERIOD as 0 when the platform headers do not provide it
```

Fifteenth D3 attempt:

```text
Commit: d79a53a
Run: 26549967383
Result: device IPA artifact produced successfully
Artifact: ios-shell-d3-first-render-device-arm64
IPA: build-products/SorrIOSShell-d3-first-render-adhoc.ipa
Asset scan: passed
Physical iPhone result: real SoRR/game runtime rendered and ran from D2-staged data
```

## Physical iPhone Proof

```text
D3 Actions run: 26549967383
Artifact: ios-shell-d3-first-render-device-arm64
IPA: build-products/SorrIOSShell-d3-first-render-adhoc.ipa
Artifact-producing commit: d79a53a
D3 docs commit before physical proof: 148ead7 [skip ci]
Install route: Windows + Sideloadly
Data source: existing D2-staged Library/Application Support/SORR data
Physical result: actual SoRR game/runtime rendered and the game ran on the physical iPhone
Audio result: no audible audio yet
Game data/assets bundled in IPA: no
Game assets committed: no
D4/D5 status: not started
```

The missing audio is not a D3 blocker. D3 explicitly allowed audio to be missing, disabled, or stubbed. Audio remains a later follow-up.

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
8. Stop after observing whether real SoRR pixels render; do not proceed to D4 controls or D5 gameplay until the physical D3 result is reported.

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
