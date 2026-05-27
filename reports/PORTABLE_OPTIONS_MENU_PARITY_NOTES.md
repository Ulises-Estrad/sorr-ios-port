# Portable Options Menu Parity Notes

Date: 2026-05-27

## Goal

Find why the prepared portable/Vita data shows a limited Options submenu instead of the original PC-style:

- Game
- Video
- Audio
- Controls

Do not start iOS work and do not implement keybinds yet.

## Short Conclusion

The menu branch is controlled by the external platform selector in `mod/system.txt`.

Original PC data:

```text
// GAME PORTS: PC, WIZ, XBOX, PSP, WII, ANDROID, HANDHELD
PC
```

Prepared portable data before this fix:

```text
// GAME PORTS: PC, WIZ, XBOX, PSP, WII, ANDROID, HANDHELD
PSP
```

`prepare.py` intentionally copied `sorr-vita-master/system.txt` over `sorr-vita-master/data/mod/system.txt`, so the prepared data inherited the `PSP` menu/platform branch.

## Fix Applied

Minimal data/config fix only:

- Changed `sorr-vita-master/system.txt` line 2 from `PSP` to `PC`.
- Changed `sorr-vita-master/data/mod/system.txt` line 2 from `PSP` to `PC`.

No keybinding UI code was implemented.
No iOS work was started.
No asset trimming was continued.

The remaining `system.txt` differences are intentionally left alone for now:

```text
Prepared: AUTO, 0, 0, 0
Original: BORDERLESS_SYNC, 3, 0, 1
```

Those are video/windowing and Xbox layout settings, not the platform selector that gates the Options submenu.

## Evidence

### DCB/Menu Strings

Commands used:

```powershell
$strings = ".\portable-tools\w64devkit\bin\strings.exe"
& $strings -a "C:\Users\ulise\Downloads\SORRv52_rev550\SORRv52\SorR.dat" |
  Select-String -Pattern "GAME|VIDEO|AUDIO|CONTROLS|CONFIGURE CONTROLS|DEFAULT CONTROLS|VITA|PSP|PC|PLATFORM|VERSION|OPTIONS"

& $strings -a ".\sorr-vita-master\data\SorR.dat" |
  Select-String -Pattern "GAME|VIDEO|AUDIO|CONTROLS|CONFIGURE CONTROLS|DEFAULT CONTROLS|VITA|PSP|PC|PLATFORM|VERSION|OPTIONS"
```

Saved outputs:

- `reports/strings_original_menu_terms.txt`
- `reports/strings_prepared_menu_terms.txt`
- `reports/strings_prepared_platform_menu_expanded.txt`

Important result: the stripped prepared DCB still contains the relevant menu/control symbols, including:

- `OPTIONS`
- `GAME_OPTIONS`
- `OPCIONES_JUEGO`
- `REMAPEAR_CONTROLES`
- `CONFIGURACION_CONTROLES`
- `BACKUP_CONTROLES`
- `LAYOUT_CONTROL`
- `OPTION_CONTROL`
- `data/controles.fpg`

So the limited submenu is not explained by the prepared DCB simply lacking menu/control code strings.

### File Layout Compare

Saved outputs:

- `reports/manifest_original.csv`
- `reports/manifest_prepared.csv`
- `reports/manifest_missing_from_prepared.txt`
- `reports/manifest_extra_in_prepared.txt`
- `reports/manifest_differences.txt`

Top-level difference:

- Original PC `SorR.dat`: `320091685` bytes
- Prepared portable `SorR.dat`: `4692183` bytes

This is expected: `extract_stub.py` extracts embedded files and writes a stripped DCB.

Files missing from prepared are mostly PC runtime binaries:

- `SorR.exe`
- `SDL.dll`
- `SDL_mixer.dll`
- BennuGD runtime DLLs
- `SorMaker.exe`
- `SorMaker.dat`
- `savegame/savestate.sor`

Prepared has the extracted asset files under `data/`, including `data/controles.fpg`.

The meaningful shared data/config difference was `mod/system.txt`.

### prepare.py

Relevant behavior:

```python
def copy_system():
    """Copy system.txt -> data/mod/system.txt"""
    src = os.path.join(BASE_DIR, "system.txt")
    dst = os.path.join(MOD_DIR, "system.txt")
    shutil.copy2(src, dst)
```

Before the fix, `sorr-vita-master/system.txt` contained `PSP`, so every rerun of `prepare.py` would restore the prepared data to the PSP branch.

### Save/Config Files

Found in original PC run:

- `savegame/savegame.sor`
- `savegame/savestate.sor`
- `savegame/trophies.sor`
- `xbox/xbox.cfg`

Found in prepared portable run:

- `savegame/savegame.sor`
- `savegame/trophies.sor`
- `xbox/xbox.cfg`

Compare result:

- `trophies.sor` hash matched.
- `xbox.cfg` hash matched.
- `savegame.sor` length matched but binary content differed.
- `savestate.sor` existed only in the original PC folder.

These are not the primary cause identified here. The platform branch file is plain text, is loaded by the game, and directly names the supported port targets.

### Runtime Detection Check

The portable CMake file does not define `TARGET_PSP` or `TARGET_VITA`.

In `core/bgdrtm/src/misc.c`, desktop Windows builds use:

```c
#ifdef _WIN32
#define _OS_ID          OS_WIN32
#endif
```

So the desktop runtime itself is not being compiled as PSP/Vita. The PSP behavior came from prepared data config.

### File-Open Evidence

Prior file-open diagnostic log shows the game reads `mod/system.txt` during launch:

```text
[PORTABLE:FILE] open request filename=mod/system.txt mode=rb
[PORTABLE:FILE] open ok direct name=mod/system.txt mode=rb
[PORTABLE:FILE] open request filename=mod/system.txt mode=rb0
[PORTABLE:FILE] open ok direct name=mod/system.txt mode=rb0
```

## Post-Fix Launch Check

Launch command:

```powershell
$root = Resolve-Path "."
$env:PATH = "$root\portable-tools\w64devkit\bin;$root\portable-deps\msys2-mingw32\mingw32\bin;$env:PATH"
Start-Process -FilePath "$root\build-portable\bgdi.exe" `
  -ArgumentList @("SorR.dat") `
  -WorkingDirectory "$root\sorr-vita-master\data"
```

Saved output:

- `reports/menu_parity_pc_system_launch_summary.txt`
- `reports/menu_parity_pc_system_stderr.log`

Result:

```text
exited=False
responding=True
title=Streets of Rage Remake - v5.2
```

Stderr:

```text
INFO: Called set_mode with 320x240x0
INFO: set_title(Streets of Rage Remake - v5.2)
INFO: Called set_mode with 320x240x0
```

The runtime still launches after the `PC` selector change.

## Current Status

Most likely fixed by the platform selector change.

Exact preventing file:

```text
sorr-vita-master/data/mod/system.txt
```

Exact template causing future prepared rebuilds to preserve the issue:

```text
sorr-vita-master/system.txt
```

Exact blocking value:

```text
PSP
```

Replacement value:

```text
PC
```

## Remaining Verification

Manual in-game verification is still needed to confirm that Options now exposes:

- Game
- Video
- Audio
- Controls

Scripted SendKeys probes did not reliably get past the intro/menu timing, so I did not treat those as authoritative UI verification.

If the submenu is still limited after this change, the next thing to inspect is how the DCB combines the `mod/system.txt` platform selector with `savegame/savegame.sor` options state. The savegame differs between original and prepared, but it is a secondary suspect after the explicit `PSP` platform selector.
