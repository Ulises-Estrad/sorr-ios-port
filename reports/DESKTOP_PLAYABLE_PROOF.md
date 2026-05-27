# Desktop Playable Proof

Date: 2026-05-27

## Status

Desktop playable proof is frozen.

Known-good result:

- Portable `bgdi.exe` builds.
- `SorR.dat` boots from `sorr-vita-master/data`.
- Window title is `Streets of Rage Remake - v5.2`.
- SDL event pump fix keeps the window responsive.
- `data/mod/system.txt` platform selector is `PC`.
- `data/savegame/savegame.sor` is the known-good keyboard-control save state.
- `data/xbox/xbox.cfg` is unchanged from the original/prepared data.
- Keyboard controls were manually verified.

## Launch Command

Preferred launcher:

```powershell
powershell -ExecutionPolicy Bypass -File .\run_sorr_portable.ps1
```

Equivalent explicit command:

```powershell
$root = "C:\Users\ulise\Documents\Codex\2026-05-26\c-users-ulise-downloads-sorrv52-rev550-2"
$env:PATH = "$root\portable-tools\w64devkit\bin;$root\portable-deps\msys2-mingw32\mingw32\bin;$env:PATH"
Push-Location "$root\sorr-vita-master\data"
& "$root\build-portable\bgdi.exe" SorR.dat
Pop-Location
```

## Working Directory

```text
C:\Users\ulise\Documents\Codex\2026-05-26\c-users-ulise-downloads-sorrv52-rev550-2\sorr-vita-master\data
```

## Required PATH Entries

```text
C:\Users\ulise\Documents\Codex\2026-05-26\c-users-ulise-downloads-sorrv52-rev550-2\portable-tools\w64devkit\bin
C:\Users\ulise\Documents\Codex\2026-05-26\c-users-ulise-downloads-sorrv52-rev550-2\portable-deps\msys2-mingw32\mingw32\bin
```

## Known-Good Files And Hashes

| File | Bytes | SHA256 |
| --- | ---: | --- |
| `build-portable/bgdi.exe` | 2264326 | `B5A01FDB354FD0037FF1276F97D6D8609ECF55C0E79EF3F9E4B5B18F08BA640E` |
| `sorr-vita-master/data/SorR.dat` | 4692183 | `4B09177B0AE87E72FFF16BE0680793136340D1CF411BE7325F14DEB2C911E65C` |
| `sorr-vita-master/data/mod/system.txt` | 238 | `EF82CB96B1E7F9C4FF3549AEC03D1C9A406E5E1FF36C1E6B0CC4BB68821F62CC` |
| `sorr-vita-master/data/savegame/savegame.sor` | 1764 | `64F3B74C32AD716C3B76D126E56AF041E7BA502A1E1247D57EDFC01892DDF572` |
| `sorr-vita-master/data/xbox/xbox.cfg` | 65 | `4E781D455E3BC9AA841D11CFA6BA7B8397770704E0FF1711A4987C85350592F9` |

## Manual Test Results

| Test | Result |
| --- | --- |
| Boots | Pass |
| Renders | Pass |
| Window responsive | Pass |
| Menu navigation | Pass |
| Keyboard controls | Pass |
| Stage load | Not recorded in this freeze pass |
| Movement | Pass, arrows |
| Attack | Pass, `C` |
| Jump | Pass, `V` |
| Special | Pass, `X` |
| Police | Pass, `B` |
| Start/confirm | Pass, `Enter` |
| Pause/back/cancel | Pass, `Escape` / `Backspace` |

Confirmed keyboard map:

```text
Arrows = movement
X = special
C = attack
V = jump
B = police
Enter = start/confirm
Escape/Backspace = back/cancel
```

## Input Diagnostic Evidence

Final passive diagnostic with `PORTABLE_INPUT_DIAG=1` showed the runtime/script polling the desired keyboard defaults:

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

Files:

```text
reports/input_diag_final_keyboard_save_summary.txt
reports/input_diag_final_keyboard_save_input_only.log
```

## Known-Good Backup

Created:

```text
out/known-good-desktop/
```

Contents:

- `bgdi.exe`
- `run_sorr_portable.ps1`
- `data-overrides/mod/system.txt`
- `data-overrides/savegame/savegame.sor`
- `data-overrides/xbox/xbox.cfg`
- `MANIFEST.md`

Large game data is not duplicated in the backup folder. `SorR.dat` is referenced by hash in the manifest.

## Known Unresolved Issue

Options submenus are still not restored.

This is no longer a blocker for the desktop playable proof or the next iOS input bridge phase.
