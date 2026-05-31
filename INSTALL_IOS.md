# iOS Install From Scratch

This guide rebuilds the current iPhone setup from zero.

## What This Repo Provides

- GitHub Actions builds a Sideloadly-ready iPhone `arm64` IPA.
- The IPA contains the iOS shell/runtime, visible crash reporting, BGM/SFX support, custom touch controls, virtual joystick movement, and the app icon.
- The repo keeps the prepared SoRR data under Git LFS so a fresh clone can recreate the import package.
- The IPA does not bundle SoRR game data or assets.
- Game data is imported through the user's iPhone Files inbox. Fresh installs may show the folder as `Streets of Rage`; older installs may still show `SorrIOSShell`.
- The install path is Windows + Sideloadly + a physical iPhone.

## Requirements

- Windows PC.
- Access to this GitHub repo.
- Sideloadly.
- Apple ID for Sideloadly signing.
- Physical iPhone with Developer Mode enabled.
- USB cable.
- Local checkout of this repo with Git LFS files pulled if regenerating the import package.

## Download And Install The Latest IPA

1. Open the GitHub repo.
2. Go to `Actions`.
3. Open the latest successful `iOS Shell` workflow run.
4. Download the latest successful device artifact. Current WAV-memory / trace-throttle playtest target:

   ```text
   Actions run: 26705635947
   Commit: 4afd24d
   Artifact: ios-shell-playtest-wav-memory-trace-throttle-device-arm64
   ```

   Previous proven baseline:

   ```text
   ios-shell-playtest-remote-ref-guard-device-arm64
   ```

5. Extract the downloaded artifact on Windows.
6. Install the IPA with Sideloadly. Current WAV-memory / trace-throttle IPA target:

   ```text
   build-products/SorrIOSShell-playtest-wav-memory-trace-throttle-adhoc.ipa
   ```

7. If iOS asks, enable Developer Mode and trust the developer profile.
8. Launch `Streets of Rage` once to create the Files folders.

## Prepare And Import Game Data Locally

From a local checkout of this repo on Windows, make sure Git LFS has downloaded the data:

```powershell
git lfs install
git lfs pull
```

Then run:

```powershell
.\tools\create_d2_import_package.ps1
```

The helper uses the repo's `sorr-vita-master/data` folder by default and creates:

```text
out/local-only/SORR_IMPORT.zip
```

Transfer and extract it on the iPhone so Files contains:

```text
On My iPhone/
  Streets of Rage/   (or SorrIOSShell on older installs)
    SORR_IMPORT/
      SorR.dat
      mod/
        system.txt
        music/
          ...
```

Relaunch `Streets of Rage`. The app will stage the imported data into:

```text
Library/Application Support/SORR
```

The game should then render and run using the staged data.

## Custom Controls

Controls are configurable in-app through `CFG`.

- Tap `CFG` to enter edit mode.
- Drag the joystick or a button to reposition it. A tap selects; controls only move after a real drag.
- Use `BIG` / `SML` to resize the selected control.
- Use `OPAC` to cycle opacity.
- Use `TXT+` / `TXT-` to show or hide gameplay button text.
- Use `RST` to reset the default layout.
- Use `DONE` to save and exit.
- `CFG` lives in the left pillar area. `START` is in the right pillar area, and `BACK` is directly below it.
- Gameplay action buttons are circular. `Back Attack` sits above `Attack` and maps to the `D` / back-attack binding.

Settings persist here:

```text
On My iPhone/Streets of Rage/SORR_DIAGNOSTICS/ios_touch_controls.ini
```

Older installs may still show the Files folder as `SorrIOSShell`.

To reset manually, either use `CFG` -> `RST` -> `DONE`, or delete `ios_touch_controls.ini` and relaunch.

## Crash Reports And Bug Reports

After a crash:

1. Reopen `Streets of Rage` once.
2. In Files, send:

   ```text
   On My iPhone/Streets of Rage/SORR_DIAGNOSTICS/LATEST_CRASH_OR_ABRUPT_EXIT_REPORT.txt
   ```

3. If more context is needed, also send:

   ```text
   On My iPhone/Streets of Rage/SORR_DIAGNOSTICS/CURRENT_SESSION_RUNTIME_LOG.txt
   On My iPhone/Streets of Rage/SORR_DIAGNOSTICS/PREVIOUS_SESSION_RUNTIME_LOG.txt
   ```

For wrong dash/run SFX after scene transitions, also send:

```text
On My iPhone/Streets of Rage/SORR_DIAGNOSTICS/CURRENT_SESSION_AUDIO_SFX_LOG.txt
```

That file is reset on each launch and logs the actual iOS audio backend's WAV load/reuse/unload/play events, including the handle id, sample path, current process, and whether an unloaded handle was later reused.

The diagnostics folder is refreshed on every app launch. `CURRENT_SESSION_RUNTIME_LOG.txt` starts fresh, the prior launch is rotated to `PREVIOUS_SESSION_RUNTIME_LOG.txt`, and old legacy `ios_*` diagnostic files are removed. If the app exits abruptly without a normal signal crash report, the next launch synthesizes `PREVIOUS_SESSION_ABRUPT_EXIT_REPORT.txt`; same-build, non-trivial exits also replace `LATEST_CRASH_OR_ABRUPT_EXIT_REPORT.txt`.

## Updating Later

1. Download a newer successful device artifact from GitHub Actions.
2. Install the new IPA over the old app with Sideloadly if possible.
3. Keep the existing staged data and `SORR_IMPORT` files on the phone.
4. If the data disappears, re-import `SORR_IMPORT.zip`.
5. If controls feel wrong after an update, reset or restore `ios_touch_controls.ini`.

## Recovery From Zero

1. Install the latest IPA from GitHub Actions with Sideloadly.
2. Recreate `out/local-only/SORR_IMPORT.zip` using `tools/create_d2_import_package.ps1`.
3. Copy/extract the `SORR_IMPORT` folder under `On My iPhone/Streets of Rage` (or `SorrIOSShell` on older installs).
4. Launch the app and wait for staging.
5. Reconfigure controls, or restore a saved `ios_touch_controls.ini`.
6. If a crash happens, reopen once and retrieve `LATEST_CRASH_OR_ABRUPT_EXIT_REPORT.txt`.

## Current Known Status

- Real SoRR renders and runs on iPhone.
- BGM works.
- SFX works.
- Virtual joystick/action touch controls work.
- Custom controls exist and persist.
- Crash reporting exists.
- Custom touch with virtual joystick movement is the current control baseline.
- The IPA remains asset-free.
- The current build is the paused/final-for-now baseline.
- Future updates should be limited to major bug fixes discovered during real play.
- Known non-blocking issue: some no-input attract/demo scenes may ignore Start/Back while showing a "press start" prompt. This does not block normal playability.
