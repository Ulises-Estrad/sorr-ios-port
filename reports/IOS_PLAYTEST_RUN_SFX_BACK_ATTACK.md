# iOS Playtest Run SFX And Back Attack Follow-Up

Status: follow-up patch target after the current playable baseline.

## Reported Issue

During physical iPhone playtesting, double-tapping forward or backward to run could play an enemy sound effect instead of the brief run sound. The issue appeared after entering some new screens, did not happen on every screen, and cleared after restarting the app.

## Patch Target

The strongest code-level suspect is stale SDL_mixer sample handle storage on iOS arm64. `mod_sound` previously used the safe handle table only on `_WIN64`; other builds converted `Mix_Chunk *` and `Mix_Music *` pointers through `int`. The iOS build now enables that handle table whenever host pointer tables are requested or the host pointer size is wider than 32 bits.

This keeps SFX/BGM enabled and does not alter game data, touch controls, or the runtime path beyond safer audio handle lookup.

## Control UI Follow-Up

This target also updates the fixed touch interface:

- action buttons draw as circular controls,
- existing joystick and action hit/input logic is preserved,
- Back Attack is added above Attack,
- Back Attack maps to Bennu key `57` / Space,
- Start and Back utility buttons keep their current positions.

## Artifact Target

```text
Artifact: ios-shell-playtest-run-sfx-back-attack-device-arm64
IPA: build-products/SorrIOSShell-playtest-run-sfx-back-attack-adhoc.ipa
Build label: ios-playtest-run-sfx-back-attack
Game data/assets bundled in IPA: no
```

## Physical Test Checklist

1. Install the IPA through the same Windows + Sideloadly path.
2. Launch using the already staged SoRR data.
3. Visit multiple screens and double-tap forward/back to run.
4. Confirm the run sound stays correct and does not become an enemy SFX.
5. Confirm circular action buttons are visible.
6. Confirm Back Attack is above Attack and triggers the Space/back-attack binding.
7. Confirm Attack, Jump, Special, Police, Start, Back, joystick, BGM, and SFX still work.

If the sound bug still occurs, reopen once after any crash and retrieve:

```text
On My iPhone/Streets of Rage/SORR_DIAGNOSTICS/ios_latest_crash_report.txt
On My iPhone/Streets of Rage/SORR_DIAGNOSTICS/ios_current_run_stability_log.txt
```
