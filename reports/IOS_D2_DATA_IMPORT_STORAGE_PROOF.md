# iOS D2 Data Import Storage Proof

Date: 2026-05-27

## Goal

D2 proves the physical iPhone shell can receive privately supplied prepared SoRR data without bundling or committing game assets.

This phase remains probe-only. It does not load or execute the Bennu runtime, render the game, add touch controls, or initialize audio.

## Status

Status: D2 physical iPhone data import/storage proof complete.

D1 remains complete at commit `dc13aaf50bc31afe9945c6b9410178642ed69e55`.

Previous D2 file-sharing proof run:

```text
Commit: 5d9036f
Run: https://github.com/Ulises-Estrad/streets-of-rage-remake-ios/actions/runs/26541978819
Device job: Build iOS shell unsigned IPA for device arm64
Device job result: success
Artifact: ios-shell-d2-data-import-device-arm64
Artifact size: 459831 bytes
IPA inside artifact: build-products/SorrIOSShell-d2-data-import-adhoc.ipa
```

The same workflow run kept the simulator shell/data-layout proof green.

Current update:

- replace the ambiguous `Documents/SORR` import folder with explicit `Documents/SORR_IMPORT`,
- treat `Documents/SORR_IMPORT` as an import inbox only,
- treat `Library/Application Support/SORR` as the canonical app data root,
- accept direct and one-folder-nested import layouts,
- show import layout and staging result on the visible status screen.

Updated D2 artifact proof:

```text
Commit: 7b395b3
Run: https://github.com/Ulises-Estrad/streets-of-rage-remake-ios/actions/runs/26543157389
Device job: Build iOS shell unsigned IPA for device arm64
Device job result: success
Artifact: ios-shell-d2-sorr-import-device-arm64
Artifact size: 461067 bytes
IPA inside artifact: build-products/SorrIOSShell-d2-sorr-import-adhoc.ipa
```

Physical iPhone D2 proof:

```text
Passing Actions run: 26543157389
Artifact: ios-shell-d2-sorr-import-device-arm64
IPA: build-products/SorrIOSShell-d2-sorr-import-adhoc.ipa
Install route: Windows + Sideloadly
Local-only import package: out/local-only/SORR_IMPORT.zip
Local package source: sorr-vita-master/data
Local package SHA256: B2E2F3901C65BE0FA3D739D3716C74C37AE13FB9BDF7535ADC487D9A3D22C72A
Physical result: iPhone D2 probe screen confirmed direct layout, staging copied, SorR.dat opened, mod/system.txt found, savegame/xbox/logs writable, probe log OK
Game execution: no
Game rendering: no
Game data/assets committed or bundled in IPA: no
```

## Chosen Import Route

The preferred document-picker ZIP route is deferred for D2 because it requires a native iOS picker bridge and ZIP extraction path.

D2 uses the approved fallback route:

- enable iOS file sharing,
- expose `Documents/SORR_IMPORT` through the Files app,
- let the user copy prepared data there,
- copy detected data into `Library/Application Support/SORR`,
- verify files and writable paths from the shell.

## IPA Artifact

Expected GitHub Actions artifact:

```text
ios-shell-d2-sorr-import-device-arm64
```

Expected IPA inside the artifact:

```text
build-products/SorrIOSShell-d2-sorr-import-adhoc.ipa
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
- resolves `Documents/SORR_IMPORT` as the file-sharing import inbox,
- resolves `Library/Application Support/SORR`,
- creates or verifies writable `savegame`, `xbox`, and `logs` directories,
- writes and reads `logs/ios_d2_data_import_probe.txt`,
- writes `Documents/SORR_IMPORT/README_D2_IMPORT.txt`,
- detects `none`, `direct`, `nested-one-folder`, or invalid import layouts,
- stages valid import data into `Library/Application Support/SORR`,
- checks for `SorR.dat`,
- checks for `mod/system.txt`,
- displays a simple SDL status screen,
- keeps the event loop responsive,
- explicitly skips Bennu runtime execution.

## Manual iPhone Import Steps

1. Download the `ios-shell-d2-sorr-import-device-arm64` artifact from GitHub Actions.
2. Extract `build-products/SorrIOSShell-d2-sorr-import-adhoc.ipa`.
3. Install the IPA on the iPhone with Sideloadly on Windows.
4. Launch `SorrIOSShell` once so iOS creates the app container and `Documents/SORR_IMPORT`.
5. On Windows, create a folder named `SORR_IMPORT`.
6. Put the prepared SoRR data contents into that folder.
7. Transfer the folder or a zip of the folder to the iPhone using iCloud Drive, iCloud.com, OneDrive, Google Drive, or another Files-visible provider.
8. On the iPhone, open the Files app and locate the transferred folder or zip.
9. If you transferred a zip, tap it once in Files to extract it.
10. Go to `On My iPhone` -> `SorrIOSShell` -> `SORR_IMPORT`.
11. Copy either the contents of the extracted `SORR_IMPORT` folder, or the one extracted folder itself, into this app `SORR_IMPORT` inbox.
12. Return to `SorrIOSShell` or relaunch it.
13. Wait for the visible status screen to show the data probe result.

Correct direct import layout:

```text
SORR_IMPORT/
  SorR.dat
  mod/
    system.txt
  savegame/
  xbox/
```

Also accepted, if the transfer tool creates one top-level folder:

```text
SORR_IMPORT/
  SomeFolder/
    SorR.dat
    mod/
      system.txt
    savegame/
    xbox/
```

Wrong nested-too-deep layout:

```text
SORR_IMPORT/
  SomeFolder/
    AnotherFolder/
      SorR.dat
```

Wrong missing/multiple layout:

```text
SORR_IMPORT/
  multiple-folders/
  other-folder/
  SorR.dat missing
```

If Files leaves you with `On My iPhone/SorrIOSShell/SORR_IMPORT/SORR_IMPORT/SorR.dat`, that is accepted as the one-folder nested layout. If it leaves you with `On My iPhone/SorrIOSShell/SORR_IMPORT/SorR.dat`, that is accepted as the direct layout.

## Local-Only Package Helper

The local Windows helper can build the import folder and zip from the prepared data already on this PC:

```powershell
powershell -ExecutionPolicy Bypass -File tools\create_d2_import_package.ps1
```

Default output:

```text
out/local-only/SORR_IMPORT/
out/local-only/SORR_IMPORT.zip
```

The helper searches known local SoRR data roots, prefers the prepared `sorr-vita-master/data` layout, copies it into ignored `out/local-only/SORR_IMPORT`, creates `out/local-only/SORR_IMPORT.zip`, and verifies these required zip entries:

```text
SORR_IMPORT/SorR.dat
SORR_IMPORT/mod/system.txt
SORR_IMPORT/mod/music/1.ogg
```

Current local package proof:

```text
Source root: sorr-vita-master/data
Zip: out/local-only/SORR_IMPORT.zip
SHA256: B2E2F3901C65BE0FA3D739D3716C74C37AE13FB9BDF7535ADC487D9A3D22C72A
Zip entries: 2430
Required entries verified: yes
```

This zip is local-only. Do not commit it, upload it, attach it to GitHub Actions, or bundle it into the IPA. `.gitignore` excludes `out/`, `out/local-only/`, `*.zip`, `SorR.dat`, `data/`, and the known game asset extensions.

D3A helper refresh:

```text
Source root: sorr-vita-master/data
Source music/BGM files found: 237
Staged music/BGM files: 237
Zip: out/local-only/SORR_IMPORT.zip
SHA256: 261190316E0D539546596738333D6752D69E209FBB8E6B7BF61B0B1DDBAB8A7D
Required entries verified: SorR.dat, mod/system.txt, mod/music/1.ogg
```

This refresh does not redefine the completed D2 physical proof. It only makes the local helper safer for D3A by ensuring the private import package includes the prepared BGM/music folder when the user refreshes the iPhone's staged data.

## Expected Visible Status

Observed physical iPhone D2 status:

```text
D2 SORR DATA PROBE
INBOX DOCUMENTS/SORR_IMPORT
DATA APP SUPPORT/SORR
LAYOUT DIRECT
STAGING COPIED
SORR.DAT FOUND OPENED
MOD/SYSTEM.TXT FOUND
PROBE LOG OK
SAVEGAME WRITABLE
XBOX WRITABLE
LOGS WRITABLE
NO GAME EXECUTION
NO GAME RENDERING
```

Before data is copied:

```text
D2 SORR DATA PROBE
INBOX DOCUMENTS/SORR_IMPORT
DATA APP SUPPORT/SORR
LAYOUT NONE
STAGING NOT STARTED
SORR.DAT NOT FOUND
MOD/SYSTEM.TXT MISSING
PROBE LOG OK
SAVEGAME WRITABLE
XBOX WRITABLE
LOGS WRITABLE
NO GAME EXECUTION
NO GAME RENDERING
```

After data is copied and staged:

```text
D2 SORR DATA PROBE
INBOX DOCUMENTS/SORR_IMPORT
DATA APP SUPPORT/SORR
LAYOUT DIRECT
STAGING COPIED
SORR.DAT FOUND OPENED
MOD/SYSTEM.TXT FOUND
PROBE LOG OK
SAVEGAME WRITABLE
XBOX WRITABLE
LOGS WRITABLE
NO GAME EXECUTION
NO GAME RENDERING
```

## Expected Logs

The app should emit these proof markers:

```text
SORR iOS shell: D2 import inbox path=
SORR iOS shell: D2 canonical data path=
SORR iOS shell: D2 import layout detected=direct
SORR iOS shell: D2 import started source=
SORR iOS shell: D2 import completed source=
SORR iOS shell: D2 staging result=copied
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
  -B build-ios-shell-d2-sorr-import-preflight `
  -G "MinGW Makefiles" `
  -DCMAKE_PREFIX_PATH="$PWD\portable-deps\msys2-mingw32\mingw32" `
  -DSORR_IOS_SHELL_USE_SDL_MAIN_HANDLED=ON
portable-tools\w64devkit\bin\cmake.exe --build build-ios-shell-d2-sorr-import-preflight --verbose
```

Result:

```text
configure: success
build: success
```

## Stop Line

Stop after the completed physical iPhone D2 import/storage proof is documented.

D3 was later explicitly requested and completed as a separate first-render proof. D2 remains the completed data import/storage milestone and should not be redefined.
