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
On My iPhone/Streets of Rage/SORR_DIAGNOSTICS/ios_audio_sfx_diagnostics.txt
```

If the app also crashes, reopen once and retrieve:

```text
On My iPhone/Streets of Rage/SORR_DIAGNOSTICS/ios_latest_crash_report.txt
On My iPhone/Streets of Rage/SORR_DIAGNOSTICS/ios_previous_run_stability_log.txt
On My iPhone/Streets of Rage/SORR_DIAGNOSTICS/ios_current_run_stability_log.txt
On My iPhone/Streets of Rage/SORR_DIAGNOSTICS/ios_previous_run_fallback_<run>.txt
```

If the scene-transition exit does not generate a signal report, the next launch archives a fallback report built from `ios_previous_run_stability_log.txt`. It only replaces `ios_latest_crash_report.txt` when the prior run is from the same build and is not just a short lifecycle/background termination.
