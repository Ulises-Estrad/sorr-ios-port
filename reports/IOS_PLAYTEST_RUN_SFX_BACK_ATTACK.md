# iOS Playtest Run SFX And Back Attack Follow-Up

Status: run-sound follow-up plus crash-report archive diagnostics available from GitHub Actions.

## Reported Issue

During physical iPhone playtesting, double-tapping forward or backward to run could play an enemy sound effect instead of the brief run sound. The issue appeared after entering some new screens, did not happen on every screen, and cleared after restarting the app.

## Patch Target

The first follow-up enabled the generic `mod_sound` handle table on iOS/64-bit builds, but the physical iPhone build uses the iOS-specific audio backend in `sorr_ios_mod_sound_stub.c`, so that patch did not hit the active playback path. The second follow-up still reproduced the wrong run sound after moving to a new scene twice.

The current patch instruments and hardens the actual iOS SDL_mixer audio backend. `LOAD_WAV` reuses an existing handle for the same path, `UNLOAD_WAV` keeps the WAV chunk alive for the app session, and every load/reuse/unload/play event is written to a Files-visible diagnostic file. This should show whether the dash/run action is playing the intended sample path or a handle that has become associated with an enemy sample after scene transitions.

## Control UI Follow-Up

This target also updates the fixed touch interface:

- action buttons draw as circular controls,
- existing joystick and action hit/input logic is preserved,
- Back Attack is added above Attack,
- Back Attack maps to Bennu key `32` / `D`,
- the Back Attack / Attack / Special column is moved closer to Jump / Police,
- tapping a selected control again deselects it in edit mode,
- Start and Back utility buttons keep their current positions.

## Artifact Target

```text
Actions run: 26676209972
Commit: 70f6c04
Artifact: ios-shell-playtest-crash-report-archive-device-arm64
IPA: build-products/SorrIOSShell-playtest-crash-report-archive-adhoc.ipa
Build label: ios-playtest-crash-report-archive
Artifact size: 1964110 bytes
Device job result: success
Simulator job result: success
Game data/assets bundled in IPA: no
```

## Physical Test Checklist

1. Install the IPA through the same Windows + Sideloadly path.
2. Launch using the already staged SoRR data.
3. Visit multiple screens. The reported repro is moving into a new scene twice.
4. Double-tap forward/back to run with Shiva SOR2 or another character whose dash/run sound is obvious.
5. If the run sound becomes an enemy SFX, stop and retrieve the SFX diagnostic file before relaunching again if possible.
6. Confirm circular action buttons are visible.
7. Confirm Back Attack is above Attack and triggers the `D` / back-attack binding.
8. Confirm Back Attack / Attack / Special sit close enough to the Jump / Police column.
9. Enter CFG, tap a control to select it, tap it again to deselect it, and confirm `DONE` / `BIG` / `SML` no longer accidentally move the selected control.
10. Confirm Attack, Jump, Special, Police, Start, Back, joystick, BGM, and SFX still work.

If the sound bug still occurs, retrieve:

```text
On My iPhone/Streets of Rage/SORR_DIAGNOSTICS/CURRENT_SESSION_AUDIO_SFX_LOG.txt
```

If the app also crashes, reopen once and retrieve:

```text
On My iPhone/Streets of Rage/SORR_DIAGNOSTICS/LATEST_CRASH_OR_ABRUPT_EXIT_REPORT.txt
On My iPhone/Streets of Rage/SORR_DIAGNOSTICS/CURRENT_SESSION_RUNTIME_LOG.txt
On My iPhone/Streets of Rage/SORR_DIAGNOSTICS/PREVIOUS_SESSION_RUNTIME_LOG.txt
On My iPhone/Streets of Rage/SORR_DIAGNOSTICS/PREVIOUS_SESSION_ABRUPT_EXIT_REPORT.txt
```

If the scene-transition exit does not generate a signal report, the next launch builds a fallback report from `PREVIOUS_SESSION_RUNTIME_LOG.txt`. It only replaces `LATEST_CRASH_OR_ABRUPT_EXIT_REPORT.txt` when the prior run is from the same build and is not just a short lifecycle/background termination.

## Current Playtest Bug Follow-Up: Remote Process Reference Guard

The newest retrieved diagnostics were from the crash-report archive build itself, not a stale older build:

```text
crash_report_type=previous-run-nosignal-fallback
previous_build=ios-playtest-crash-report-archive
current_build=ios-playtest-crash-report-archive
previous_build_matches_current=1
previous_has_sdl_terminating=0
previous_short_lifecycle_termination=0
```

The run had no `signal=` line and no SDL terminating event. BGM/SFX were healthy, and the tail showed scene-transition activity plus many stale process reference guards, so the most likely remaining failure class was a Bennu VM remote-process lookup hitting `Process not active` and calling `exit(0)`.

Current artifact target:

```text
Actions run: 26676881894
Commit: 06a2745
Artifact: ios-shell-playtest-remote-ref-guard-device-arm64
IPA: build-products/SorrIOSShell-playtest-remote-ref-guard-adhoc.ipa
Build label: ios-playtest-remote-ref-guard
Artifact size: 1964199 bytes
Device job result: success
Simulator job result: success
Game data/assets bundled in IPA: no
```

Patch contents:

- keeps custom controls, BGM/SFX, app icon/name, SFX diagnostics, crash-report archives, and the asset-free IPA packaging,
- leaves desktop/x64 behavior unchanged,
- on iOS only, guards stale `MN_REMOTE*` and `MN_GET_REMOTE*` process dereferences,
- logs `runtime_enemigo_lookup_guard reason=remote-* ...` breadcrumbs,
- returns a safe zero/no-op value instead of letting stale destroyed process IDs terminate the app through the interpreter's fatal `Process not active` path.

Test focus: install this IPA, play through the scene transition that abruptly exited before, and if it still exits or crashes, reopen once and send `LATEST_CRASH_OR_ABRUPT_EXIT_REPORT.txt`.

## Current Playtest Follow-Up: Clear Diagnostics Filenames

The next artifact keeps the same playable baseline and remote process guard, but makes the Files-visible diagnostic folder less confusing. On each app launch it removes legacy `ios_*` diagnostic files, rotates the previous launch to one clear file, and starts a fresh current-session log.

Current artifact target:

```text
Actions run: 26677510668
Commit: fd61bd1
Artifact: ios-shell-playtest-clear-diagnostics-device-arm64
IPA: build-products/SorrIOSShell-playtest-clear-diagnostics-adhoc.ipa
Build label: ios-playtest-clear-diagnostics
Artifact size: 1965861 bytes
Device job result: success
Simulator job result: success
Game data/assets bundled in IPA: no
```

Files to send after a crash or abrupt exit:

```text
On My iPhone/Streets of Rage/SORR_DIAGNOSTICS/LATEST_CRASH_OR_ABRUPT_EXIT_REPORT.txt
On My iPhone/Streets of Rage/SORR_DIAGNOSTICS/CURRENT_SESSION_RUNTIME_LOG.txt
On My iPhone/Streets of Rage/SORR_DIAGNOSTICS/PREVIOUS_SESSION_RUNTIME_LOG.txt
```
