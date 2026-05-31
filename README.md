# Streets of Rage Remake iOS

Current baseline: the real SoRR runtime runs on a physical iPhone from SoRR data staged on the device. The game renders, BGM works, SFX works, the virtual joystick/action controls work, custom controls persist, and crash reporting/guards remain active.

This repo is now the source of truth for the iOS shell/runtime build. It is no longer just an early proof-only experiment.

## Current Roadmap

- Current: paused/final-for-now playable baseline with a clear-diagnostics follow-up. The app has the `Streets of Rage` display name, app icon, virtual joystick, pillar-safe CFG button, right-side Start/Back buttons, persistent custom layout settings, and Files-visible crash reports.
- Next: no active feature work. Patch only major bugs if they show up during real playtesting.

Remaining work is no longer planned as an active roadmap. Future changes should be limited to major bug fixes found during normal gameplay. Minor polish/control tuning can wait unless it blocks play.

Latest physical playtest result: the `ios-shell-playtest-real-point-guard-device-arm64` IPA fixed the reported gun-shot crash and the Stage 6 beach/water startup crash. A full SoR2 route clear with Axel completed without clear bugs or random crashes.

Current follow-up target: `ios-shell-playtest-post-no-cargues-trace-device-arm64` from Actions run `26702197600` keeps the same playable baseline and adds direct visible tracing around the current no-signal scene-transition exit. The latest stage-transition report showed `signal=none`, no SDL terminating event, `runtime_exit_guards=0`, and the last durable marker as `destroy NO_CARGUES#70014`, so this target logs `post_no_cargues_*` markers, native calls/returns, and frame boundaries into the Files-visible runtime log.

Known non-blocking issue: some no-input attract/demo scenes may ignore Start/Back presses even while showing a "press start" prompt. This does not block normal playability because the current build can be started, controlled, and played through; leave this alone unless it becomes a practical blocker or a clear crash/repro case appears.

## Install From Scratch

Use [INSTALL_IOS.md](INSTALL_IOS.md) to recover from zero, download the latest IPA, regenerate the SoRR import package from Git LFS data, reset custom controls, and collect crash reports.

The current playtest baseline is tracked in [reports/IOS_CURRENT_PLAYTEST_BASELINE.md](reports/IOS_CURRENT_PLAYTEST_BASELINE.md).

## Data And IPA Rule

The repo now keeps the prepared SoRR data under Git LFS so a fresh clone can regenerate the iPhone import package. The GitHub Actions IPA still does not bundle `SorR.dat`, `data/`, FPG/WAV/OGG/SMK/PNG game assets, prepared data, logs, zips, or generated IPAs. The app continues to use the Files import/staging route on the iPhone.
