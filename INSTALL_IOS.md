# Install Streets of Rage Remake on iPhone

## 1. Download the IPA

**[⬇ Download Streets-of-Rage-Remake-iOS.ipa](https://github.com/Ulises-Estrad/streets-of-rage-remake-ios/releases/latest/download/Streets-of-Rage-Remake-iOS.ipa)**

Or open [the latest release](https://github.com/Ulises-Estrad/streets-of-rage-remake-ios/releases/latest), expand **Assets**, and select **Streets-of-Rage-Remake-iOS.ipa**. Do not select the automatically generated Source code ZIP.

The IPA includes the complete prepared game data and music. You do not need another ZIP, an import folder, or a separate game-data transfer.

## 2. Sign and install

Use your existing Sideloadly setup or another IPA signing tool to sign and install the downloaded IPA on your iPhone. It is ad-hoc packaged, not App Store or device-provisioned software; downloading it in Safari alone does not install it.

For an update, retain the same bundle identifier and signing identity and install over the existing app. Do not uninstall first if you want to retain its saves. Certificate renewal requirements depend on your signing method.

## 3. Play

Open **Streets of Rage**. Game data is available immediately. The joystick is on the left; action buttons are on the right and Start is at the top right.

Pair a PS5 DualSense through iPhone Bluetooth settings to use a controller. The touch overlay disappears when connected and returns after disconnection. See the [button map](README.md#ps5-dualsense).

## Existing saves and old imports

On its first bundled-data launch, the app copies existing private saves from the old import-based installation into its new writable save directory. It does not overwrite saves on later launches. This migration requires retaining the same installed app container.

Game assets stay in the app bundle. Private links point to them so the runtime can load files without creating another asset copy. Old `SORR_IMPORT` folders and older private data are left untouched; this build does not require them and does not delete them.

The old CFG editor and `ios_touch_controls.ini` settings are no longer used.

## Report a problem

If the game crashes, reopen it once and retrieve the latest report from **Files → On My iPhone → Streets of Rage → SORR_DIAGNOSTICS**. Include the release tag, iPhone model, iOS version, character, stage, and whether you were using touch or DualSense.

For this release, physical checks should cover a fresh install without imports, an update retaining saves, each action button, connecting a pre-paired DualSense during play, disconnecting while holding a direction/button, reconnecting, and background/resume.
