# SoRR iPhone Port

Current baseline: the real SoRR runtime runs on a physical iPhone from private data staged on the device. The game renders, BGM works, SFX works, fixed touch controls work, D4b custom controls persist, and D3S crash reporting/guards remain active.

This repo is now the source of truth for the iOS shell/runtime build. It is no longer just an early proof-only experiment.

## Current Roadmap

- Current: GitHub consolidation, app icon, reinstall-from-scratch docs, and the latest stable custom-touch IPA from GitHub Actions.
- Next: playtest the game in free time and debug major bugs found during real play.

Remaining work is mostly control tuning, polish, and bug fixes discovered during normal gameplay. The old no-input attract/demo crash appears route-specific and is not blocking active playtesting right now.

## Install From Scratch

Use [INSTALL_IOS.md](INSTALL_IOS.md) to recover from zero, download the latest IPA, import private SoRR data locally, reset custom controls, and collect crash reports.

## Important Asset Rule

The GitHub Actions IPA does not bundle `SorR.dat`, `data/`, FPG/WAV/OGG/SMK/PNG game assets, prepared data, logs, zips, or generated IPAs. Private game data stays local and is imported onto the iPhone by the user.
