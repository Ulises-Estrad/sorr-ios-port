# iOS D2 Data Import Storage Proof

Date: 2026-05-27

## Goal

D2 proves the physical iPhone shell can receive privately supplied prepared SoRR data without bundling or committing game assets.

This phase remains probe-only. It does not load or execute the Bennu runtime, render the game, add touch controls, or initialize audio.

## Status

Status: D2 GitHub Actions device IPA artifact proof complete; physical iPhone import/storage test pending.

D1 remains complete at commit `dc13aaf50bc31afe9945c6b9410178642ed69e55`.

Proof run:

```text
Commit: 5d9036f
Run: https://github.com/Ulises-Estrad/sorr-ios-port/actions/runs/26541978819
Device job: Build iOS shell unsigned IPA for device arm64
Device job result: success
Artifact: ios-shell-d2-data-import-device-arm64
Artifact size: 459831 bytes
IPA inside artifact: build-products/SorrIOSShell-d2-data-import-adhoc.ipa
```

The same workflow run kept the simulator shell/data-layout proof green.

## Chosen Import Route

The preferred document-picker ZIP route is deferred for D2 because it requires a native iOS picker bridge and ZIP extraction path.

D2 uses the approved fallback route:

- enable iOS file sharing,
- expose `Documents/SORR` through the Files app,
- let the user copy prepared data there,
- copy detected data into `Library/Application Support/SORR`,
- verify files and writable paths from the shell.

## IPA Artifact

Expected GitHub Actions artifact:

```text
ios-shell-d2-data-import-device-arm64
```

Expected IPA inside the artifact:

```text
build-products/SorrIOSShell-d2-data-import-adhoc.ipa
```

The IPA must contain no:

- `SorR.dat`
- `data/`
- `.fpg`
- `.wav`
- `.ogg`
- `.smk`
- `.png`
- prepared game data

## App Behavior

On launch the D2 shell:

- resolves the app bundle resource root,
- resolves `Documents/SORR` for file-sharing import,
- resolves `Library/Application Support/SORR`,
- creates or verifies writable `savegame`, `xbox`, and `logs` directories,
- writes and reads `logs/ios_d2_data_import_probe.txt`,
- writes `Documents/SORR/README_D2_IMPORT.txt`,
- checks for `SorR.dat`,
- checks for `mod/system.txt`,
- displays a simple SDL status screen,
- keeps the event loop responsive,
- explicitly skips Bennu runtime execution.

## Manual iPhone Import Steps

1. Download the `ios-shell-d2-data-import-device-arm64` artifact from GitHub Actions.
2. Extract `build-products/SorrIOSShell-d2-data-import-adhoc.ipa`.
3. Install the IPA on the iPhone with Sideloadly on Windows.
4. Launch `SorrIOSShell` once so iOS creates the app container and `Documents/SORR`.
5. Open the iOS Files app.
6. Go to `On My iPhone` -> `SorrIOSShell` -> `SORR`.
7. Copy the contents of the prepared SoRR data folder into `SORR`.

Expected copied layout:

```text
Documents/SORR/SorR.dat
Documents/SORR/mod/system.txt
Documents/SORR/savegame/
Documents/SORR/xbox/
```

The shell also accepts this fallback layout:

```text
Documents/SORR/data/SorR.dat
Documents/SORR/data/mod/system.txt
```

8. Return to `SorrIOSShell` or relaunch it.
9. Wait for the visible status screen to show the data probe result.

## Expected Visible Status

Before data is copied:

```text
D2 SORR DATA PROBE
FILE SHARING ROUTE
WAITING FOR DATA IMPORT
FILES APP: SORRIOSSHELL/SORR
SORR.DAT NOT FOUND
MOD/SYSTEM.TXT MISSING
SAVEGAME XBOX LOGS WRITABLE
NO GAME EXECUTION
```

After data is copied and staged:

```text
D2 SORR DATA PROBE
FILE SHARING ROUTE
IMPORT STARTED
IMPORT COMPLETED
SORR.DAT FOUND OPENED
MOD/SYSTEM.TXT FOUND
SAVEGAME XBOX LOGS WRITABLE
NO GAME EXECUTION
```

## Expected Logs

The app should emit these proof markers:

```text
SORR iOS shell: D2 file sharing import path=
SORR iOS shell: D2 import started source=
SORR iOS shell: D2 import completed source=
SORR iOS shell: D2 SorR.dat found/opened path=
SORR iOS shell: D2 required data file mod/system.txt found/opened path=
SORR iOS shell: D2 probe log write/read ok path=
SORR iOS shell: D2 data import/storage proof ready
SORR iOS shell: Bennu runtime intentionally skipped in D2
SORR iOS shell: SorR.dat intentionally not loaded or executed in D2
```

## Local Preflight

Windows syntax/link preflight passed with the D2 source:

```powershell
$env:PATH = "$PWD\portable-tools\w64devkit\bin;$env:PATH"
portable-tools\w64devkit\bin\cmake.exe -S sorr-vita-master/cmake/ios `
  -B build-ios-shell-d2-preflight `
  -G "MinGW Makefiles" `
  -DCMAKE_PREFIX_PATH="$PWD\portable-deps\msys2-mingw32\mingw32" `
  -DSORR_IOS_SHELL_USE_SDL_MAIN_HANDLED=ON
portable-tools\w64devkit\bin\cmake.exe --build build-ios-shell-d2-preflight --verbose
```

Result:

```text
configure: success
build: success
```

## Stop Line

Stop after the D2 IPA artifact is produced and the manual import steps are documented.

Do not proceed to D3/rendering until the physical iPhone D2 import/storage probe is manually tested and reported.
