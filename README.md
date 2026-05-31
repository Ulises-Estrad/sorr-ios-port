# Streets of Rage Remake iOS

Current baseline: the real SoRR runtime runs on a physical iPhone from SoRR data staged on the device. The game renders, BGM works, SFX works, the virtual joystick/action controls work, custom controls persist, and crash reporting/guards remain active.

This repo is now the source of truth for the iOS shell/runtime build. It is no longer just an early proof-only experiment.

## Current Roadmap

- Current: paused/final-for-now playable baseline. The app has the `Streets of Rage` display name, app icon, virtual joystick, pillar-safe CFG button, right-side Start/Back buttons, persistent custom layout settings, and Files-visible crash reports.
- Next: no active feature work. Patch only major bugs if they show up during real playtesting.

Remaining work is no longer planned as an active roadmap. Future changes should be limited to major bug fixes found during normal gameplay. Minor polish/control tuning can wait unless it blocks play.

Latest physical playtest result: the `ios-shell-playtest-wav-memory-trace-throttle-device-arm64` IPA fixed the latest scene-transition crash and removed the logging-induced gameplay slowdown. Earlier fixes in the current baseline also fixed the reported gun-shot crash and the Stage 6 beach/water startup crash. A full SoR2 route clear with Axel completed without clear bugs or random crashes.

Current stable artifact: `ios-shell-playtest-wav-memory-trace-throttle-device-arm64` from Actions run `26705635947`. It keeps the same playable baseline, throttles post-`NO_CARGUES` tracing, and moves iOS `LOAD_WAV` onto a memory-backed path like the working OGG music loader. Crash reports include the last WAV path/status.

Current color-fix candidate: `ios-shell-playtest-palette-handle-cleanup-device-arm64` from Actions run `26721338436`. This keeps the playable baseline and targets the transition-only Shiva/player/effect miscolor by clearing stale 64-bit palette handles when palettes are destroyed and ignoring invalid instance palette overrides.

Known non-blocking issue: some no-input attract/demo scenes may ignore Start/Back presses even while showing a "press start" prompt. This does not block normal playability because the current build can be started, controlled, and played through; leave this alone unless it becomes a practical blocker or a clear crash/repro case appears.

## Install From Scratch

Use [INSTALL_IOS.md](INSTALL_IOS.md) to recover from zero, download the latest IPA, regenerate the SoRR import package from Git LFS data, reset custom controls, and collect crash reports.

The current playtest baseline is tracked in [reports/IOS_CURRENT_PLAYTEST_BASELINE.md](reports/IOS_CURRENT_PLAYTEST_BASELINE.md).

## Data And IPA Rule

The repo now keeps the prepared SoRR data under Git LFS so a fresh clone can regenerate the iPhone import package. The GitHub Actions IPA still does not bundle `SorR.dat`, `data/`, FPG/WAV/OGG/SMK/PNG game assets, prepared data, logs, zips, or generated IPAs. The app continues to use the Files import/staging route on the iPhone.
