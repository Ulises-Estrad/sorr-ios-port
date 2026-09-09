# Streets of Rage Remake iOS

## [⬇ Download IPA — all game assets included](https://github.com/Ulises-Estrad/streets-of-rage-remake-ios/releases/latest/download/Streets-of-Rage-Remake-iOS.ipa)

**Click the download link above to get `Streets-of-Rage-Remake-iOS.ipa`.** No separate game-data download or Files import is needed.

[Release page and download details](https://github.com/Ulises-Estrad/streets-of-rage-remake-ios/releases/latest) · [Installation instructions](INSTALL_IOS.md)

Install the IPA with **Sideloadly** or another IPA signing tool. The downloadable IPA needs signing for your iPhone; opening the download in Safari does not install it. Keep the same bundle identifier when updating to retain your existing saves.

## Touch controls

The left side has a joystick. The right side has these rows:

| | | |
| --- | --- | --- |
| Police | | |
| Attack | Jump | Special |
| Back Attack | Series | Combo |

**Start** is in the top-right corner. Buttons and joystick use the matching [Kenney Mobile Controls](https://kenney.nl/assets/mobile-controls) artwork, with pressed feedback and multi-touch support. The old CFG button and layout editor have been removed; old layout files are ignored.

## PS5 DualSense

Pair your DualSense in **iPhone Settings → Bluetooth**. Touch controls disappear while a DualSense is connected and return automatically if it disconnects, including during gameplay.

| DualSense input | Action |
| --- | --- |
| Left stick / D-pad | Move |
| Cross (×) | Jump |
| Square (□) | Attack |
| Triangle (△) | Special |
| L1 | Police |
| R1 | Back Attack |
| Circle (○) | Series |
| R2 | Combo |
| Options | Start / Pause |
| Create | Menu Back |

The bridge uses the prepared game's keyboard bindings. Keep the game's P1 input set to keyboard with its default bindings.

## Game data and saves

All prepared game assets, music, and `SorR.dat` ship inside the IPA. The app reads them from its bundle; it does not extract another full copy into Files. Only saves and small configuration files need writable private storage.

When updating the same installed app, existing saves from the older import-based build are migrated without overwriting progress. Old imported folders are left untouched. There is no need to create `SORR_IMPORT` for this version.

## Builds and validation

Successful `main` builds publish the IPA to **GitHub Releases**, which supplies the download link at the top of this page. The workflow builds device and simulator apps, tests touch/controller handoff and save migration, and verifies every bundled game file against a SHA-256 manifest.

The prior runtime baseline completed all four main-game routes with SoR2 Axel and SoR2 Shiva. The new touch layout, physical Bluetooth disconnect/reconnect, and bundled-data startup still need an iPhone playtest; automated tests do not establish physical-device gameplay coverage.

## Credits

Touch artwork: Kenney, **Mobile Controls 1.0**, CC0. Original selected PNGs, license, and reproducible embedding script are included in the repository. Runtime history and earlier playtest evidence remain in [`reports/`](reports/).
