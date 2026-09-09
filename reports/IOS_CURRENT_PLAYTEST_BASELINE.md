# iOS Current Playtest Baseline

**Current baseline: [ios-88](https://github.com/Ulises-Estrad/streets-of-rage-remake-ios/releases/tag/ios-88), confirmed working on the owner's iPhone.**

## Owner-reported result

The owner tested all requested changes on their iPhone and confirmed they work: the updated touch controls and layout, removal of CFG, DualSense mapping and automatic touch-control switching, and the IPA with bundled game assets.

The owner's confirmation was: "i tested all our changes and it works fine". This is a physical-device report from the owner, separate from automated CI validation. No device model, iOS version, play duration, or individually recorded test matrix was supplied.

## Current features

- Kenney Mobile Controls artwork, joystick on the left, and action buttons on the right.
- Right-side rows: Police; Attack / Jump / Special; Back Attack / Series / Combo.
- Start at the top right; the old CFG layout editor is removed.
- DualSense left stick/D-pad for movement, Cross for Jump, Square for Attack, Triangle for Special, L1 for Police, R1 for Back Attack, Circle for Series, and R2 for Combo.
- Touch controls hide while DualSense is connected and return after disconnection.
- All game assets are bundled in the IPA; no separate Files import is required.
- Private writable saves and existing-save migration remain in place.
- A prominent direct IPA download link is at the top of the README.

## Release and automated evidence

- Release: `ios-88`
- Build commit: `09f891f05236ae9d5515ebc8985b04600e6e96f3`
- [Actions run 34293530309](https://github.com/Ulises-Estrad/streets-of-rage-remake-ios/actions/runs/34293530309): device build, simulator launch, input/storage tests, and release publication passed.
- IPA: `Streets-of-Rage-Remake-iOS.ipa`, 345,022,114 bytes.
- All 2,413 bundled game files were verified against the SHA-256 manifest.

## Earlier runtime coverage

The earlier palette-handle baseline completed all four main-game routes with SoR2 Axel and SoR2 Shiva. That is historical route coverage, not a claim that all routes were replayed for ios-88. Earlier fixes and evidence are preserved in the [historical baseline report](IOS_PALETTE_HANDLE_BASELINE_HISTORY.md).
