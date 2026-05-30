# iOS Playtest Run SFX And Back Attack Follow-Up

Status: second run-sound/edit-controls follow-up target ready for GitHub Actions.

## Reported Issue

During physical iPhone playtesting, double-tapping forward or backward to run could play an enemy sound effect instead of the brief run sound. The issue appeared after entering some new screens, did not happen on every screen, and cleared after restarting the app.

## Patch Target

The first follow-up enabled the safe handle table on iOS/64-bit builds, but physical testing still reproduced the wrong run sound. The updated suspect is stale/reused WAV sample identity after screen cleanup: an old game-side sample handle can survive while the underlying runtime unload/reload path reuses sample slots for a different enemy sound.

The current patch keeps iOS WAV chunk handles stable by filename for the app session. `LOAD_WAV` reuses an existing handle for the same path, and `UNLOAD_WAV` keeps the handle/chunk alive on iOS instead of clearing the slot for a different sample. This keeps SFX/BGM enabled and does not alter game data, touch controls, or the runtime path beyond safer audio handle identity.

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
Actions run: 26673765318
Commit: 8e926e3
Artifact: ios-shell-playtest-stable-sfx-edit-controls-device-arm64
IPA: build-products/SorrIOSShell-playtest-stable-sfx-edit-controls-adhoc.ipa
Build label: ios-playtest-stable-sfx-edit-controls
Artifact size: 1961595 bytes
Device job result: success
Simulator job result: success
Game data/assets bundled in IPA: no
```

## Physical Test Checklist

1. Install the IPA through the same Windows + Sideloadly path.
2. Launch using the already staged SoRR data.
3. Visit multiple screens and double-tap forward/back to run.
4. Confirm the run sound stays correct and does not become an enemy SFX.
5. Confirm circular action buttons are visible.
6. Confirm Back Attack is above Attack and triggers the `D` / back-attack binding.
7. Confirm Back Attack / Attack / Special sit close enough to the Jump / Police column.
8. Enter CFG, tap a control to select it, tap it again to deselect it, and confirm `DONE` / `BIG` / `SML` no longer accidentally move the selected control.
9. Confirm Attack, Jump, Special, Police, Start, Back, joystick, BGM, and SFX still work.

If the sound bug still occurs, reopen once after any crash and retrieve:

```text
On My iPhone/Streets of Rage/SORR_DIAGNOSTICS/ios_latest_crash_report.txt
On My iPhone/Streets of Rage/SORR_DIAGNOSTICS/ios_current_run_stability_log.txt
```
