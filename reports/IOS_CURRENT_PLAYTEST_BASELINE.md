# iOS Current Playtest Baseline

Status: current playable iPhone baseline, paused/final for now, with one run-sound diagnostic follow-up target ready for GitHub Actions.

This project is now a playable iPhone port baseline, not an early proof-only experiment. Historical milestone reports remain in `reports/` as evidence, but current work should use this playtest baseline framing. Active feature work is paused; future changes should be limited to major bug fixes found during normal playtesting.

## Current State

- Real SoRR renders and runs on a physical iPhone from data staged on the device.
- BGM works.
- SFX works.
- Virtual joystick movement works.
- Attack, Jump, Special, Police, Start, and Back controls work.
- Custom control layout, size, opacity, and text visibility settings persist locally.
- Crash reporting and runtime guards remain active.
- The app icon and `Streets of Rage` display name are wired into the iPhone build.
- The IPA remains asset-free; game data is imported locally through the iPhone Files route.
- Latest physical playtest fix: gun firing works and Stage 6 beach/water startup no longer crashes after the GET_REAL_POINT guard.
- Latest route playtest: the SoR2 path was cleared with Axel with no clear bugs or random crashes observed afterward.
- Current follow-up target: preserve the audio SFX diagnostics and add a fallback latest-crash report for abrupt scene-transition exits that do not reach the signal handler.

## Current Workflow Target

```text
Actions run: 26626858854
Commit: cd08c9b
Artifact: ios-shell-playtest-real-point-guard-device-arm64
IPA: build-products/SorrIOSShell-playtest-real-point-guard-adhoc.ipa
Build label: ios-playtest-real-point-guard
Artifact size: 1959448 bytes
Device job result: success
Simulator job result: success
Game data/assets bundled in IPA: no
```

Previous follow-up target:

```text
Actions run: 26672932784
Commit: 3b7811e
Artifact: ios-shell-playtest-run-sfx-back-attack-device-arm64
IPA: build-products/SorrIOSShell-playtest-run-sfx-back-attack-adhoc.ipa
Build label: ios-playtest-run-sfx-back-attack
Artifact size: 1961521 bytes
Device job result: success
Simulator job result: success
Game data/assets bundled in IPA: no
```

Current follow-up target:

```text
Actions run: 26675725620
Commit: 555ba50
Artifact: ios-shell-playtest-fallback-crash-report-device-arm64
IPA: build-products/SorrIOSShell-playtest-fallback-crash-report-adhoc.ipa
Build label: ios-playtest-fallback-crash-report
Artifact size: 1963861 bytes
Device job result: success
Simulator job result: success
Game data/assets bundled in IPA: no
```

## Control Layout

- `CFG`: small opaque button in the left pillar-safe area.
- `START`: small utility button in the right pillar-safe area.
- `BACK`: small utility button below `START`, lowered enough that the visible rectangles do not overlap after renderer minimum sizing. Start/Back use the same `52x36` minimum visual size as the CFG utility button. On a 430-point-tall iPhone landscape drawable, `START` begins near `41px` and draws to `77px`; `BACK` now begins near `95px`, leaving about `18px` of visible space.
- Virtual joystick: lower-left movement area, mapped to the same Bennu arrow keys.
- Action buttons: right-side circular gameplay buttons mapped to the existing Bennu key defaults.
- Back Attack: circular button above Attack, mapped to Bennu key `32` / `D`.

In edit mode, tapping selects a control, tapping the same selected control again deselects it, and rearranging requires dragging. This prevents accidental layout moves from simple taps or config button presses.

## Current Roadmap

Current: final-for-now playable baseline plus a focused run-sound diagnostic build. The latest proven baseline includes the GET_REAL_POINT pointer-output guard and crash-report version 3.

Next: test the run-sound diagnostic build on the physical iPhone. After that, return to free-time playtesting and patch only major bugs one by one if they are found during real play.

## Latest Physical Playtest Result

The `ios-shell-playtest-real-point-guard-device-arm64` artifact fixed the reported gun-shot crash and the Stage 6 startup crash. Guns fire normally, and Stage 6 reaches the beach/water stage without crashing.

The same baseline also cleared the SoR2 route with Axel in physical iPhone playtesting. No clear bugs or random crashes were observed from that run.

## Known Non-Blocking Issues

- Some no-input attract/demo scenes may ignore Start/Back presses even while the game displays a "press start" prompt.
- The issue appears tied to specific attract/demo paths rather than normal active play.
- This is not considered important enough to pursue right now because the game can be started, controlled, and played normally.
- Revisit only if it becomes a practical blocker or produces a clearer bug/crash report.

## Debug Files

If the app crashes, reopen once and retrieve:

```text
On My iPhone/Streets of Rage/SORR_DIAGNOSTICS/ios_latest_crash_report.txt
```

If the crash/exit does not reach the signal handler, the next launch should synthesize that file with `crash_report_type=previous-run-nosignal-fallback` and include the previous run tail.

If more context is needed, also retrieve:

```text
On My iPhone/Streets of Rage/SORR_DIAGNOSTICS/ios_current_run_stability_log.txt
```

For wrong dash/run SFX after scene transitions, retrieve:

```text
On My iPhone/Streets of Rage/SORR_DIAGNOSTICS/ios_audio_sfx_diagnostics.txt
```
