# iOS Current Playtest Baseline

Status: current playable iPhone baseline.

This project is now a playable iPhone port baseline, not an early proof-only experiment. Historical milestone reports remain in `reports/` as evidence, but current work should use this playtest baseline framing.

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

## Control Layout

- `CFG`: small opaque button in the left pillar-safe area.
- `START`: small utility button in the right pillar-safe area.
- `BACK`: small utility button below `START`, lowered enough that the visible rectangles do not overlap after renderer minimum sizing. Start/Back use the same `52x36` minimum visual size as the CFG utility button. On a 430-point-tall iPhone landscape drawable, `START` begins near `41px` and draws to `77px`; `BACK` now begins near `95px`, leaving about `18px` of visible space.
- Virtual joystick: lower-left movement area, mapped to the same Bennu arrow keys.
- Action buttons: right-side gameplay buttons mapped to the existing Bennu key defaults.

In edit mode, tapping selects a control, but rearranging requires dragging. This prevents accidental layout moves from simple taps.

## Current Roadmap

Current: normal playtest bug fixing. The latest IPA includes the GET_REAL_POINT pointer-output guard and crash-report version 3.

Next: playtest the game in free time and fix major bugs one by one as they are found during real play.

## Debug Files

If the app crashes, reopen once and retrieve:

```text
On My iPhone/Streets of Rage/SORR_DIAGNOSTICS/ios_latest_crash_report.txt
```

If more context is needed, also retrieve:

```text
On My iPhone/Streets of Rage/SORR_DIAGNOSTICS/ios_current_run_stability_log.txt
```
