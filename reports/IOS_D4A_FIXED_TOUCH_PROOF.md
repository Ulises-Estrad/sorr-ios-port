# iOS D4a Fixed Touch And Crash Reports

Status: GitHub-side D4a IPA produced successfully; ready for physical iPhone Sideloadly test.

D4a starts after the completed D3 first-render proof and D3A audio proof. It keeps the D2-staged data path, real BGM/SFX, and D3S stability guards active while adding fixed on-screen controls and cleaner crash reporting.

## Scope

Included:

- fixed on-screen touch controls,
- direct Bennu key-state injection,
- multitouch hold support for D-pad plus action buttons,
- simple translucent SDL overlay,
- Files-visible touch diagnostics,
- cleaner current-run stability logs,
- compact latest crash report.

Not included:

- D4b control customization,
- D5 gameplay polish,
- App Store/TestFlight signing,
- bundled `SorR.dat`,
- bundled `data/` or prepared game assets,
- audio disabling or attract/demo suppression.

## Artifact Target

GitHub Actions device artifact:

```text
ios-shell-d4a-fixed-touch-guarded-device-arm64
```

IPA inside artifact:

```text
build-products/SorrIOSShell-d4a-fixed-touch-guarded-adhoc.ipa
```

The IPA must remain asset-free. CI should reject `SorR.dat`, `data/`, `.fpg`, `.wav`, `.ogg`, `.smk`, `.png`, prepared data, logs, and build outputs inside the IPA.

## Touch Layout

Left side:

- `UP` -> Bennu key `72`
- `DOWN` -> Bennu key `80`
- `LEFT` -> Bennu key `75`
- `RIGHT` -> Bennu key `77`

Right side:

- `ATK` -> Bennu key `46` (`C`)
- `JUMP` -> Bennu key `47` (`V`)
- `SPC` -> Bennu key `45` (`X`)
- `POL` -> Bennu key `48` (`B`)

Top:

- `START` -> Bennu key `28` (Enter)
- `BACK` -> Bennu key `1` (Escape), with key `14` (Backspace) held as a fallback

The bridge writes directly into the iOS `mod_key` path instead of relying on SDL keyboard-event synthesis. Touch down/up events are logged with button name, mapped key, fallback key, and injection path.

## Crash Reports

Files-visible diagnostics folder:

```text
On My iPhone/SorrIOSShell/SORR_DIAGNOSTICS
```

Primary crash file to send after a crash:

```text
ios_latest_crash_report.txt
```

Current launch log:

```text
ios_current_run_stability_log.txt
```

Previous launch log:

```text
ios_previous_run_stability_log.txt
```

Full rolling log remains available:

```text
ios_d3_runtime_stability_probe.txt
```

On launch, the app rotates the previous current-run log, starts a clean current-run log, writes a launch delimiter with build/artifact labels, and preserves the latest crash report until a newer crash replaces it.

On a catchable crash signal, the compact report records signal number, ticks/runtime, stage, current process, last process pointer state, last lookup, lookup guard count, lifecycle/family/render rings, latest runtime snapshot, recent destroyed process ring, and the last current-launch diagnostic lines.

## Manual Test

1. Keep the D2-staged data on the iPhone.
2. Download `ios-shell-d4a-fixed-touch-guarded-device-arm64` from GitHub Actions.
3. Extract the artifact on Windows.
4. Install `build-products/SorrIOSShell-d4a-fixed-touch-guarded-adhoc.ipa` with Sideloadly.
5. Launch `SorrIOSShell`.
6. Confirm real SoRR rendering still appears.
7. Confirm BGM/SFX still work.
8. Test menu navigation, start, movement, attack, jump, special, police, pause/start, and back/menu.
9. Leave the app running for 10-15 minutes when practical.
10. If it crashes, reopen once and retrieve `ios_latest_crash_report.txt` from Files.
11. Send `ios_current_run_stability_log.txt` only if extra context is needed.

## Expected Result

The app should remain D3-compatible while becoming playable enough for fixed-control testing. If the five-minute attract/demo crash still occurs, the next report should be much smaller and should isolate the latest crash instead of requiring a long historical log search.

## GitHub-Side Artifact Proof

Produced successfully:

```text
Actions run: 26607711467
Patch commit: dc64c13
Artifact: ios-shell-d4a-fixed-touch-guarded-device-arm64
IPA: build-products/SorrIOSShell-d4a-fixed-touch-guarded-adhoc.ipa
Artifact size: 811502 bytes
Device job result: success
Simulator job result: success
Game data/assets bundled in IPA: no
```

This artifact is the current D4a physical iPhone test build. It keeps real BGM/SFX enabled, keeps the D2-staged data path, keeps D3S guards/diagnostics, and adds the fixed touch overlay plus current-run/latest-crash log files.

## D4a Visual Overlay/Viewport Follow-Up

Physical testing of run `26607711467` confirmed the D4a input bridge is working: touch hitboxes, Bennu key-state injection, rendering, BGM, and SFX all worked on the iPhone. The remaining D4a issue is visual composition only.

Observed visual issues:

- touch controls worked but were not visible,
- a clipped rectangle appeared near the bottom middle of the screen,
- a half-clipped square appeared in the top-right,
- the 16:9 pillar bars were white instead of black.

The follow-up keeps the working touch hitboxes and key mappings unchanged. It only changes overlay rendering and viewport state handling:

- force the game-frame clear color to black before copying the game texture,
- draw the D4a overlay after the game texture copy and before `SDL_RenderPresent`,
- reset SDL logical size, viewport, clip rect, scale, and blend mode before drawing the overlay,
- draw high-contrast translucent button fills, double outlines, and readable labels in full-window coordinates,
- restore the previous SDL renderer state after overlay drawing.

Expected follow-up artifact:

```text
ios-shell-d4a-visible-touch-viewport-device-arm64
```

Expected IPA inside artifact:

```text
build-products/SorrIOSShell-d4a-visible-touch-viewport-adhoc.ipa
```

Manual test focus:

1. Install the follow-up IPA with Sideloadly.
2. Confirm side bars are black.
3. Confirm the touch controls are visibly drawn and still match the working hitboxes.
4. Confirm the bottom-middle rectangle and top-right half square are gone.
5. Confirm BGM/SFX and gameplay control still work.
6. If a crash occurs, reopen once and send `ios_latest_crash_report.txt`.

## D4a Visible Touch / Viewport Artifact Proof

Produced successfully:

```text
Actions run: 26608727747
Patch commit: ec24f1d
Artifact: ios-shell-d4a-visible-touch-viewport-device-arm64
IPA: build-products/SorrIOSShell-d4a-visible-touch-viewport-adhoc.ipa
Artifact size: 812321 bytes
Device job result: success
Simulator job result: success
Game data/assets bundled in IPA: no
```

This is the current D4a physical iPhone test build for the visual overlay and viewport fix. It keeps the already-working touch hitboxes/key injection, BGM/SFX, D2 staged-data path, D3S guards, and crash-report files. It only changes renderer state handling and visible overlay drawing.

## D4a Compact D-Pad Follow-Up

Physical testing of the visible-overlay artifact confirmed that the game is playable, BGM/SFX work, and the overlay is visible. The remaining control issue is movement feel: the four independent movement rectangles can leave an old direction held while sliding to a new direction.

The compact D-pad follow-up keeps the working action buttons and key injection path, but replaces the four movement rectangles with one lower-left D-pad control:

- one active D-pad touch owns movement,
- movement is recomputed from the touch position relative to the D-pad center,
- previous direction keys are released before new direction keys are pressed,
- a center deadzone allows neutral,
- cardinal directions are preferred, with diagonals only when the touch is intentionally in a diagonal zone,
- D-pad transitions are logged with old/new masks and key up/down events.

Artifact target:

```text
ios-shell-d4a-dpad-refine-device-arm64
```

IPA target:

```text
build-products/SorrIOSShell-d4a-dpad-refine-adhoc.ipa
```

Manual test focus:

1. Slide Right to Left without lifting: Right should release and Left should press.
2. Slide Right to Up: Right should release unless the touch is intentionally in an Up+Right diagonal zone.
3. Hold a D-pad direction and press Attack/Jump.
4. Confirm Attack, Jump, Special, Police, Start, and Back still work.
5. Confirm BGM/SFX still work.

GitHub-side artifact proof:

```text
Actions run: 26611068603
Patch commit: 95ae6d2
Artifact: ios-shell-d4a-dpad-refine-device-arm64
IPA: build-products/SorrIOSShell-d4a-dpad-refine-adhoc.ipa
Artifact size: 813146 bytes
Device job result: success
Simulator job result: success
Game data/assets bundled in IPA: no
Physical iPhone result: pending manual D-pad slide test
```

## D4a/D4b Combined Control Pass

Physical testing of the compact D-pad artifact showed that the D-pad was too stiff: a held direction could feel locked to the first touch-down direction instead of fluidly following finger motion. The next pass keeps the working action buttons and Bennu key injection, but recomputes the D-pad direction continuously on touch motion.

It also adds D4b-lite customization:

- `CFG` enters edit mode,
- drag the D-pad or any button to reposition it,
- `BIG` / `SML` resize the selected control,
- `OPAC` cycles overlay opacity,
- `HIDE` / `SHOW` toggles overlay visibility,
- `RST` resets defaults,
- `DONE` saves and exits edit mode,
- settings persist in `On My iPhone/SorrIOSShell/SORR_DIAGNOSTICS/ios_touch_controls.ini`.

Artifact target:

```text
ios-shell-d4b-custom-touch-device-arm64
```

IPA target:

```text
build-products/SorrIOSShell-d4b-custom-touch-adhoc.ipa
```

Manual test focus:

1. Slide Right to Left without lifting: Right should release and Left should press.
2. Slide Right to Up: Right should release unless the touch is intentionally in a diagonal zone.
3. Hold a D-pad direction and press Attack/Jump.
4. Tap `CFG`, drag/resize controls, adjust opacity, save with `DONE`, relaunch, and confirm settings persist.
5. Confirm BGM/SFX, crash reporting, and the D2 staged-data path still work.

GitHub-side D4b custom-touch artifact proof:

```text
Actions run: 26612339231
Patch commit: f5beb9a
Artifact: ios-shell-d4b-custom-touch-device-arm64
IPA: build-products/SorrIOSShell-d4b-custom-touch-adhoc.ipa
Artifact size: 798 KB
Device job result: success
Simulator job result: success
Game data/assets bundled in IPA: no
Physical iPhone result: pending manual D-pad/customization test
```

## Consolidated D4b Baseline With App Icon

D4b custom touch is the current usable baseline. The next device artifact adds the temporary app icon while keeping the same real-game runtime, BGM/SFX, custom controls, D3S crash reports, and asset-free packaging.

```text
ios-shell-d4b-custom-touch-icon-device-arm64
build-products/SorrIOSShell-d4b-custom-touch-icon-adhoc.ipa
```

GitHub-side consolidated icon artifact proof:

```text
Actions run: 26613847549
Commit: 444d0a0
Artifact: ios-shell-d4b-custom-touch-icon-device-arm64
IPA: build-products/SorrIOSShell-d4b-custom-touch-icon-adhoc.ipa
Artifact size: 1955535 bytes
Device job result: success
Simulator job result: success
Game data/assets bundled in IPA: no
```

Current project status:

- real SoRR renders and runs on iPhone,
- BGM works,
- SFX works,
- touch controls work,
- custom controls exist and persist,
- crash reporting exists,
- remaining work is playtesting, control tuning, polish, and major bug fixes found during real play.

## D4b Joystick Control Follow-Up

Physical testing of the app-icon custom-touch build confirmed that the game is playable, BGM/SFX work, and custom controls persist, but the compact D-pad still did not feel natural enough. The next control pass preserves the working action buttons, Start/Back buttons, Bennu key injection, custom layout persistence, D3S guards, and crash reporting, while replacing the visible/behavioral movement control with a virtual joystick.

The joystick still maps to the same movement keys:

- Up -> Bennu key 72
- Down -> Bennu key 80
- Left -> Bennu key 75
- Right -> Bennu key 77

The joystick behavior is:

- one active movement touch owns the joystick,
- finger motion continuously recomputes the direction mask,
- old direction keys release before new direction keys press,
- a center deadzone prevents accidental drift,
- diagonals require intentional off-axis movement,
- action buttons remain on their existing working path,
- `CFG` has a larger hit target to improve edit-mode entry reliability.

Artifact target:

```text
ios-shell-d4b-joystick-controls-icon-device-arm64
```

IPA target:

```text
build-products/SorrIOSShell-d4b-joystick-controls-icon-adhoc.ipa
```

Manual test focus:

1. Confirm the icon remains visible on the iPhone home screen.
2. Confirm `CFG` enters edit mode consistently.
3. Slide the joystick from Right to Left without lifting: Right should release and Left should press.
4. Slide from Right to Up: Right should release unless the stick is deliberately held in a diagonal zone.
5. Hold joystick movement plus Attack/Jump/Special/Police.
6. Confirm layout resize/reposition/opacity/visibility still persist after relaunch.
7. Confirm BGM/SFX, D3S crash reporting, and the D2 staged-data path still work.

GitHub-side D4b joystick-control icon artifact proof:

```text
Actions run: 26617138664
Commit: 64dee98
Artifact: ios-shell-d4b-joystick-controls-icon-device-arm64
IPA: build-products/SorrIOSShell-d4b-joystick-controls-icon-adhoc.ipa
Artifact size: 1956172 bytes
Device job result: success
Simulator job result: success
Game data/assets bundled in IPA: no
Physical iPhone result: pending manual joystick/CFG test
```

## D4b CFG / Joystick Config Fix

Physical testing of the joystick-control icon build showed that the movement control path is now the right direction, but CFG/DONE taps were unreliable. The likely cause is iOS delivering both a touch event and a synthetic mouse event for one tap: CFG opened on the touch and then immediately closed on the synthetic mouse event hitting DONE, while DONE could close and immediately re-open CFG.

The next control artifact keeps the joystick, action buttons, BGM/SFX, D3S guards, crash reporting, app icon, and asset-free packaging, with these control UI fixes:

- suppress synthetic mouse events briefly after real touch events,
- move the config toolbar to a vertical left-side strip so it no longer covers Start/Back,
- remove the overlay hide/show behavior from the toolbar,
- replace it with `TXT+` / `TXT-` for gameplay button text labels,
- hide gameplay button text by default while still showing labels in edit mode,
- preserve the existing configurable joystick/button layout file.

Artifact target:

```text
ios-shell-d4b-config-joystick-icon-device-arm64
```

IPA target:

```text
build-products/SorrIOSShell-d4b-config-joystick-icon-adhoc.ipa
```

Manual test focus:

1. Tap `CFG`: edit mode should open consistently.
2. Tap `DONE`: edit mode should close consistently.
3. Confirm the config buttons do not cover Start/Back.
4. Confirm `TXT+` / `TXT-` toggles gameplay button labels while config labels remain visible.
5. Confirm joystick movement and action buttons still work.

GitHub-side D4b CFG/joystick config artifact proof:

```text
Actions run: 26618164911
Commit: b587ee3
Artifact: ios-shell-d4b-config-joystick-icon-device-arm64
IPA: build-products/SorrIOSShell-d4b-config-joystick-icon-adhoc.ipa
Artifact size: 1956085 bytes
Device job result: success
Simulator job result: success
Game data/assets bundled in IPA: no
Physical iPhone result: pending manual CFG/DONE/toolbar/text-label test
```

## D4b Joystick Release / Pillar CFG Follow-Up

Physical testing of the CFG/joystick artifact showed that the CFG hitbox was improved but still had odd dead zones, and the joystick could stick in the last pressed direction until CFG reset the controls. The likely joystick issue is that iOS can provide a synthetic mouse-up as the practical release signal for a touch; the previous duplicate-mouse filter suppressed that release.

The next artifact keeps joystick/action mappings, BGM/SFX, D3S reporting, app icon, and asset-free packaging, with these fixes:

- duplicate synthetic mouse down/move events remain suppressed,
- synthetic mouse up can release any stuck joystick/action state,
- mouse-up with no tracked touch slot releases orphaned pressed controls,
- the small `CFG` button is moved into the left pillar-safe area for a 16:9 game on iPhone 16 Plus style screens,
- `CFG` is opaque by default and inset from the curved top-left corner.

Artifact target:

```text
ios-shell-d4b-joystick-release-cfg-device-arm64
```

IPA target:

```text
build-products/SorrIOSShell-d4b-joystick-release-cfg-adhoc.ipa
```

Manual test focus:

1. Press/release joystick directions and confirm movement never sticks after lifting.
2. Hold a direction for more than a second, release, and confirm the key releases.
3. Confirm CFG is small, opaque, in the left pillar area, and not clipped by the curved corner.
4. Confirm CFG still opens edit mode and DONE exits edit mode.
5. Confirm Start/Back, action buttons, BGM/SFX, and crash reporting remain intact.

GitHub-side D4b joystick-release / pillar CFG artifact proof:

```text
Actions run: 26618833984
Commit: e4cbf2b
Artifact: ios-shell-d4b-joystick-release-cfg-device-arm64
IPA: build-products/SorrIOSShell-d4b-joystick-release-cfg-adhoc.ipa
Artifact size: 1956468 bytes
Device job result: success
Simulator job result: success
Game data/assets bundled in IPA: no
App display name: Streets of Rage
Physical iPhone result: pending manual joystick-release/CFG placement test
```
