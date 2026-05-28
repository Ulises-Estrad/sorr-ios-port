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
- draw high-contrast translucent button fills, double outlines, readable labels, and a small `D4A TOUCH` marker,
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
