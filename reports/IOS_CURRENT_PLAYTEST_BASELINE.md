# iOS Current Playtest Baseline

Status: current playable iPhone baseline, paused/final for now. The WAV-memory / trace-throttle artifact is the current fixed playtest baseline.

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
- Latest physical playtest fix: the WAV-memory / trace-throttle artifact fixed the latest scene-transition crash and removed the logging-induced gameplay slowdown.
- Earlier physical playtest fix: gun firing works and Stage 6 beach/water startup no longer crashes after the GET_REAL_POINT guard.
- Latest route playtest: the SoR2 path was cleared with Axel with no clear bugs or random crashes observed afterward.
- Current fixed baseline: keep the stale remote process guard, clear diagnostic filenames, runtime-exit guard, and post-`NO_CARGUES` breadcrumbs, but throttle that tracing because the previous Files-visible runtime log reached hundreds of MB and caused gameplay slowdown. The no-signal report narrowed the scene-transition exit to a `LOAD_WAV` call after `NO_CARGUES` teardown, so iOS WAV loading now uses a memory-backed `SDL_RWops` path like the working OGG music loader.
- Current color-fix candidate: `ios-shell-playtest-argb-pixel-format-device-arm64` targets the newly reported Shiva sprite blue/miscolored palette issue by aligning the iOS 32-bit SDL texture/surface format with Bennu's internal `0xAARRGGBB` pixel layout.

## Current Workflow Target

```text
Actions run: 26705635947
Commit: 4afd24d
Artifact: ios-shell-playtest-wav-memory-trace-throttle-device-arm64
IPA: build-products/SorrIOSShell-playtest-wav-memory-trace-throttle-adhoc.ipa
Build label: ios-playtest-wav-memory-trace-throttle
Artifact size: 1970563 bytes
Device job result: success
Simulator job result: success
Game data/assets bundled in IPA: no
```

This target keeps custom controls, BGM/SFX, the app icon/name, clear Files-visible diagnostics, stale process guards, stage-transition diagnostics, and runtime-exit guards. It limits `post_no_cargues_*` visible logging to focused native/process breadcrumbs, decodes string parameters for native calls, caps frame breadcrumbs, and records `LOAD_WAV` memory-read/decode status in the Files-visible runtime log and crash report.

Physical result: this artifact fixed the scene-transition crash reported after the post-`NO_CARGUES` trace build, and gameplay no longer shows the slowdown caused by the overly large runtime log.

## Current Color-Fix Candidate

```text
Actions run: 26720654116
Commit: df8bca7
Artifact: ios-shell-playtest-argb-pixel-format-device-arm64
IPA: build-products/SorrIOSShell-playtest-argb-pixel-format-adhoc.ipa
Build label: ios-playtest-argb-pixel-format
Target issue: Shiva sprite appears blue/miscolored on physical iPhone.
Artifact size: 1970719 bytes
Device job result: success
Simulator job result: success
Game data/assets bundled in IPA: no
```

Patch intent: keep the current playable baseline, BGM/SFX, custom controls, app icon/name, and crash reporting intact. The only runtime-render change is iOS-specific: the 32-bit SDL texture/surface format is `ARGB8888`, matching Bennu's internal `gr_rgb` / `gr_rgba` and bitmap format layout of `0xAARRGGBB`. The build also logs the SDL pixel masks used for future color diagnostics.

Previous follow-up target:

```text
Actions run: 26702197600
Commit: b0e3123
Artifact: ios-shell-playtest-post-no-cargues-trace-device-arm64
IPA: build-products/SorrIOSShell-playtest-post-no-cargues-trace-adhoc.ipa
Build label: ios-playtest-post-no-cargues-trace
Artifact size: 1969549 bytes
Device job result: success
Simulator job result: success
Game data/assets bundled in IPA: no
```

That target proved useful but was too verbose for active play: the uploaded `PREVIOUS_SESSION_RUNTIME_LOG.txt` reached roughly 489 MB, and the final durable trace was a `LOAD_WAV` native call from `FASE1` with no matching return.

Previous follow-up target:

```text
Actions run: 26691684119
Commit: 2774d57
Artifact: ios-shell-playtest-stage-transition-diagnostics-device-arm64
IPA: build-products/SorrIOSShell-playtest-stage-transition-diagnostics-adhoc.ipa
Build label: ios-playtest-stage-transition-diagnostics
Artifact size: 1968875 bytes
Device job result: success
Simulator job result: success
Game data/assets bundled in IPA: no
```

Previous follow-up target:

```text
Actions run: 26690777842
Commit: 7568eca
Artifact: ios-shell-playtest-runtime-exit-guard-device-arm64
IPA: build-products/SorrIOSShell-playtest-runtime-exit-guard-adhoc.ipa
Build label: ios-playtest-runtime-exit-guard
Artifact size: 1968239 bytes
Device job result: success
Simulator job result: success
Game data/assets bundled in IPA: no
```

Previous gameplay-crash target:

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

Previous follow-up target:

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

That older crash-report follow-up prevented stale old-build reports or very short lifecycle/background exits from overwriting the latest report, but its per-run fallback filenames are superseded by the clearer current/previous/latest names in the current target.

Previous follow-up target:

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

This follow-up removes old visible `ios_*` diagnostic files on launch, rotates the prior app session to `PREVIOUS_SESSION_RUNTIME_LOG.txt`, starts a fresh `CURRENT_SESSION_RUNTIME_LOG.txt`, and uses `LATEST_CRASH_OR_ABRUPT_EXIT_REPORT.txt` as the single report to send after a crash or abrupt exit.

Current follow-up target:

```text
Actions run: 26690777842
Commit: 7568eca
Artifact: ios-shell-playtest-runtime-exit-guard-device-arm64
IPA: build-products/SorrIOSShell-playtest-runtime-exit-guard-adhoc.ipa
Build label: ios-playtest-runtime-exit-guard
Artifact size: 1968239 bytes
Device job result: success
Simulator job result: success
Game data/assets bundled in IPA: no
```

The latest attached diagnostics showed a same-build no-signal fallback from `ios-playtest-clear-diagnostics`, no SDL terminating event, healthy BGM/SFX, and an abrupt exit during scene transition/startup after `NO_CARGUES`. This target keeps the playable baseline, ignores script-level `EXIT()` requests on iOS, logs `runtime_exit_request` breadcrumbs for Bennu/runtime exit paths, and converts interpreter hard `exit(0)` paths into signal-backed reports instead of silent app exits.

Previous follow-up target:

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

The latest Downloads crash bundle showed a same-build `previous-run-nosignal-fallback` report, no `signal=`, no SDL terminating event, healthy BGM/SFX, and an abrupt exit during a scene transition with many stale effect/HUD/process references. The iOS interpreter now guards stale remote process dereferences by logging `reason=remote-*` lookup diagnostics and returning a safe zero/no-op value instead of letting the VM call `exit(0)` for `Process not active`.

## Control Layout

- `CFG`: small opaque button in the left pillar-safe area.
- `START`: small utility button in the right pillar-safe area.
- `BACK`: small utility button below `START`, lowered enough that the visible rectangles do not overlap after renderer minimum sizing. Start/Back use the same `52x36` minimum visual size as the CFG utility button. On a 430-point-tall iPhone landscape drawable, `START` begins near `41px` and draws to `77px`; `BACK` now begins near `95px`, leaving about `18px` of visible space.
- Virtual joystick: lower-left movement area, mapped to the same Bennu arrow keys.
- Action buttons: right-side circular gameplay buttons mapped to the existing Bennu key defaults.
- Back Attack: circular button above Attack, mapped to Bennu key `32` / `D`.

In edit mode, tapping selects a control, tapping the same selected control again deselects it, and rearranging requires dragging. This prevents accidental layout moves from simple taps or config button presses.

## Current Roadmap

Current: final-for-now playable baseline plus stale remote-process reference guards and runtime-exit guard diagnostics. The latest proven baseline includes the GET_REAL_POINT pointer-output guard, SFX diagnostics, clear current/previous/latest diagnostic files, iOS-only remote process dereference guards, and iOS runtime-exit breadcrumbs.

Next: continue free-time playtesting and patch only major bugs one by one if they are found during real play.

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
On My iPhone/Streets of Rage/SORR_DIAGNOSTICS/LATEST_CRASH_OR_ABRUPT_EXIT_REPORT.txt
```

If the crash/exit does not reach the signal handler, the next launch should synthesize that file with `crash_report_type=previous-run-nosignal-fallback` and include the previous run tail. The previous no-signal report is also saved as `PREVIOUS_SESSION_ABRUPT_EXIT_REPORT.txt`.

If more context is needed, also retrieve:

```text
On My iPhone/Streets of Rage/SORR_DIAGNOSTICS/CURRENT_SESSION_RUNTIME_LOG.txt
On My iPhone/Streets of Rage/SORR_DIAGNOSTICS/PREVIOUS_SESSION_RUNTIME_LOG.txt
```

For wrong dash/run SFX after scene transitions, retrieve:

```text
On My iPhone/Streets of Rage/SORR_DIAGNOSTICS/CURRENT_SESSION_AUDIO_SFX_LOG.txt
```

The Files-visible diagnostics are refreshed on each app launch. Old legacy `ios_*` logs and per-run fallback archives are removed from `SORR_DIAGNOSTICS`, the current session starts fresh, and the previous session is rotated to a single clear previous-session file.
