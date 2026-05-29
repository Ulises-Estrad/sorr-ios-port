# SoRR iPhone Port

Current baseline: the real SoRR runtime runs on a physical iPhone from SoRR data staged on the device. The game renders, BGM works, SFX works, fixed touch controls work, D4b custom controls persist, and D3S crash reporting/guards remain active.

This repo is now the source of truth for the iOS shell/runtime build. It is no longer just an early proof-only experiment.

## Current Roadmap

- Current: GitHub consolidation is complete; the app-icon custom-touch IPA is available from GitHub Actions run `26613847549`.
- Next: playtest the game in free time and debug major bugs found during real play.

Remaining work is mostly control tuning, polish, and bug fixes discovered during normal gameplay. The old no-input attract/demo crash appears route-specific and is not blocking active playtesting right now.

## Install From Scratch

Use [INSTALL_IOS.md](INSTALL_IOS.md) to recover from zero, download the latest IPA, regenerate the SoRR import package from Git LFS data, reset custom controls, and collect crash reports.

## Data And IPA Rule

The repo now keeps the prepared SoRR data under Git LFS so a fresh clone can regenerate the iPhone import package. The GitHub Actions IPA still does not bundle `SorR.dat`, `data/`, FPG/WAV/OGG/SMK/PNG game assets, prepared data, logs, zips, or generated IPAs. The app continues to use the Files import/staging route on the iPhone.
