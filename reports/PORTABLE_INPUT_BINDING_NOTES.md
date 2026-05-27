# Portable Input Binding Notes

Date: 2026-05-27

## Goal

Stop chasing the Options submenu for now and identify why Player 1 is treated as a missing gamepad instead of the PC keyboard defaults:

- Arrows = movement
- X = special
- C = attack
- V = jump
- B = police
- Enter = start

## Short Conclusion

The persistent binding state is in `savegame/savegame.sor`, not `xbox/xbox.cfg`.

`xbox/xbox.cfg` is byte-identical between the original PC folder and the prepared folder. The prepared `savegame.sor` differs from the original PC `savegame.sor` and does not cause the script to poll the X/C/V/B keyboard action keys during the passive input diagnostic window.

Final prepared data now uses:

```text
sorr-vita-master/data/savegame/savegame.sor = original PC savegame/savegame.sor
sorr-vita-master/data/xbox/xbox.cfg = original prepared xbox.cfg, unchanged
```

## Backups

Prepared files were backed up before swaps:

```text
reports/control_swap_backups/
```

The exact backup folder for this run is recorded in:

```text
reports/latest_control_swap_backup.txt
```

Backed up files:

- `prepared_savegame.sor`
- `prepared_xbox.cfg`
- `original_pc_savegame.sor`
- `original_pc_xbox.cfg`

## File Compare

### savegame/savegame.sor

Original PC:

```text
SHA256=36F21709BD89D21A7585CFE63C1C388E9C8CCE0E7D10A2E41DFD956366DDCF2F
Length=1764
```

Prepared before fix:

```text
SHA256=6F95B00115CCD7BC2C61DAF9A800761086D4EB1F7589F42EE16F8D387973E0F6
Length=1764
```

The files have the same length but different stored values.

Saved dword diff:

```text
reports/savegame_dword_differences_original_vs_prepared.txt
```

Important keyboard-looking values:

```text
ByteOffset 1196: original=45 prepared=106
ByteOffset 1200: original=46 prepared=105
ByteOffset 1204: original=47 prepared=104
ByteOffset 1208: original=48 prepared=113
```

Bennu key constants from `libkey.c`:

```text
45 = X
46 = C
47 = V
48 = B
```

This matches the desired PC action buttons.

### xbox/xbox.cfg

Original PC and prepared hashes match:

```text
SHA256=4E781D455E3BC9AA841D11CFA6BA7B8397770704E0FF1711A4987C85350592F9
Length=65
```

Contents:

```text
xpos=0
ypos=0
xstretch=0
ystretch=0
flickerfilter=1
720p=1
```

`xbox.cfg` is not the differentiating input binding file.

## Swap Tests

All launch probes used:

```powershell
$root = Resolve-Path "."
$env:PATH = "$root\portable-tools\w64devkit\bin;$root\portable-deps\msys2-mingw32\mingw32\bin;$env:PATH"
Start-Process -FilePath "$root\build-portable\bgdi.exe" `
  -ArgumentList @("SorR.dat") `
  -WorkingDirectory "$root\sorr-vita-master\data"
```

### Both Files From Original PC

Copied:

- original PC `savegame/savegame.sor`
- original PC `xbox/xbox.cfg`

Result:

```text
responding=True
title=Streets of Rage Remake - v5.2
```

Saved:

- `reports/control_swap_both_summary.txt`
- `reports/control_swap_both_t50.png`
- `reports/control_swap_both_after_keys.png`
- `reports/control_swap_both_native_summary.txt`
- `reports/control_swap_both_native_before_keys.png`
- `reports/control_swap_both_native_after_keys.png`

Automated key injection was not reliable enough to treat as proof of in-game control state. It reached the title screen, but synthetic key events did not consistently advance the SDL window.

### savegame.sor Only

Copied:

- original PC `savegame/savegame.sor`

Kept:

- prepared `xbox/xbox.cfg`

Result:

```text
responding=True
title=Streets of Rage Remake - v5.2
savehash=36F21709BD89D21A7585CFE63C1C388E9C8CCE0E7D10A2E41DFD956366DDCF2F
xboxhash=4E781D455E3BC9AA841D11CFA6BA7B8397770704E0FF1711A4987C85350592F9
```

Saved:

- `reports/control_swap_save_only_summary.txt`

### xbox.cfg Only

Copied:

- original PC `xbox/xbox.cfg`

Kept:

- prepared `savegame/savegame.sor`

Result:

```text
responding=True
title=Streets of Rage Remake - v5.2
savehash=6F95B00115CCD7BC2C61DAF9A800761086D4EB1F7589F42EE16F8D387973E0F6
xboxhash=4E781D455E3BC9AA841D11CFA6BA7B8397770704E0FF1711A4987C85350592F9
```

Saved:

- `reports/control_swap_xbox_only_summary.txt`

Since `xbox.cfg` is identical, this is not expected to change P1 binding behavior.

## Diagnostics Added

Portable-only input diagnostics were added behind `PORTABLE_RUNTIME_DIAG` and runtime env var `PORTABLE_INPUT_DIAG=1`.

Files changed:

- `sorr-vita-master/modules/mod_key/mod_key.c`
- `sorr-vita-master/modules/libjoy/libjoy.c`

Diagnostics log:

- detected joystick count
- opened joystick names/buttons/axes/hats
- first `JOY_NUMBER` call
- `JOY_SELECT` requests
- invalid selected/requested joystick accesses
- first poll of watched keyboard defaults:
  - Enter `28`
  - X `45`
  - C `46`
  - V `47`
  - B `48`
  - Up `72`
  - Left `75`
  - Right `77`
  - Down `80`

Build log:

```text
reports/control_diag_build_1.log
```

Build result:

```text
LASTEXITCODE=0
```

## Diagnostic Results

### Prepared savegame.sor

Saved:

- `reports/input_diag_prepared_save_summary.txt`
- `reports/input_diag_prepared_save_input_only.log`

Input log:

```text
[PORTABLE:INPUT] SDL_NumJoysticks=0
[PORTABLE:INPUT] KEY poll code=72 initial=0
[PORTABLE:INPUT] KEY poll code=80 initial=0
[PORTABLE:INPUT] KEY poll code=75 initial=0
[PORTABLE:INPUT] KEY poll code=77 initial=0
[PORTABLE:INPUT] KEY poll code=28 initial=0
[PORTABLE:INPUT] JOY_NUMBER max=0 selected=-1
```

Prepared save polls arrows and Enter, but does not poll X/C/V/B in this probe.

### Original PC savegame.sor

Saved:

- `reports/input_diag_original_pc_save_summary.txt`
- `reports/input_diag_original_pc_save_input_only.log`

Input log:

```text
[PORTABLE:INPUT] SDL_NumJoysticks=0
[PORTABLE:INPUT] JOY_NUMBER max=0 selected=-1
[PORTABLE:INPUT] KEY poll code=72 initial=0
[PORTABLE:INPUT] KEY poll code=80 initial=0
[PORTABLE:INPUT] KEY poll code=75 initial=0
[PORTABLE:INPUT] KEY poll code=77 initial=0
[PORTABLE:INPUT] KEY poll code=28 initial=0
[PORTABLE:INPUT] KEY poll code=45 initial=0
[PORTABLE:INPUT] KEY poll code=46 initial=0
[PORTABLE:INPUT] KEY poll code=47 initial=0
[PORTABLE:INPUT] KEY poll code=48 initial=0
```

Original PC save causes the runtime/script to poll the full desired keyboard defaults.

### Final Prepared State

After copying the original PC `savegame.sor` into prepared data, final diagnostic run:

Saved:

- `reports/input_diag_final_keyboard_save_summary.txt`
- `reports/input_diag_final_keyboard_save_input_only.log`

Final log:

```text
[PORTABLE:INPUT] SDL_NumJoysticks=0
[PORTABLE:INPUT] JOY_NUMBER max=0 selected=-1
[PORTABLE:INPUT] KEY poll code=72 initial=0
[PORTABLE:INPUT] KEY poll code=80 initial=0
[PORTABLE:INPUT] KEY poll code=75 initial=0
[PORTABLE:INPUT] KEY poll code=77 initial=0
[PORTABLE:INPUT] KEY poll code=28 initial=0
[PORTABLE:INPUT] KEY poll code=45 initial=0
[PORTABLE:INPUT] KEY poll code=46 initial=0
[PORTABLE:INPUT] KEY poll code=47 initial=0
[PORTABLE:INPUT] KEY poll code=48 initial=0
```

Final launch:

```text
responding=True
title=Streets of Rage Remake - v5.2
savehash=36F21709BD89D21A7585CFE63C1C388E9C8CCE0E7D10A2E41DFD956366DDCF2F
```

## Current Status

Player 1 keyboard defaults are now restored at the data level by using the original PC `savegame.sor`.

The final prepared file is:

```text
sorr-vita-master/data/savegame/savegame.sor
```

Manual verification with a real keyboard should confirm that character select no longer reports missing Gamepad 1 and that:

- arrows move
- X triggers special
- C attacks/selects
- V jumps/backs as appropriate
- B triggers police
- Enter starts

If the game still shows Gamepad 1 missing with this final save, the next diagnostic step is to navigate to character select with a real keyboard while `PORTABLE_INPUT_DIAG=1` is active and inspect whether the script starts issuing invalid `JOY_GET*` calls after that screen loads.
