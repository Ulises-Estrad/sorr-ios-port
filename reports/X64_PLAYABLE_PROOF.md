# X64 Playable Proof

Date: 2026-05-27

## Status

The x64 desktop runtime is now the known-good 64-bit base for the iOS path.

Final x64 executable:

- Path: `build-portable-x64/bgdi.exe`
- SHA256: `4FE38BC20771428C6C986E72A09441DB9DE246170A3B009EE61528C413729592`

Proof result:

- Boots `SorR.dat`.
- Opens the SDL window titled `Streets of Rage Remake - v5.2`.
- Renders title/city/menu flow.
- Reaches character select.
- Reaches route/stage flow.
- Reaches live gameplay as Axel.
- Stays responsive during long-running probes.
- Keyboard controls are visible through Bennu `mod_key` diagnostics.
- Gameplay timing now matches the practical 32-bit baseline.

## Launch Command

Required PATH:

```powershell
$env:PATH="$PWD\portable-tools\w64devkit-x64\bin;$PWD\portable-deps\msys2-mingw64\mingw64\bin;$env:PATH"
```

Working directory:

```text
C:\Users\ulise\Documents\Codex\2026-05-26\c-users-ulise-downloads-sorrv52-rev550-2\out\x64-launch-data
```

Launch:

```powershell
cd out\x64-launch-data
..\..\build-portable-x64\bgdi.exe SorR.dat
```

Frozen-runtime launch equivalent:

```powershell
cd out\x64-launch-data
..\known-good-x64\bgdi.exe SorR.dat
```

## Known-Good Data State

Launch data root:

```text
C:\Users\ulise\Documents\Codex\2026-05-26\c-users-ulise-downloads-sorrv52-rev550-2\out\x64-launch-data
```

The x64 launch folder uses local game data by reference:

- `out\x64-launch-data\data` is a junction to `sorr-vita-master\data\data`.
- `out\x64-launch-data\mod` is a junction to `sorr-vita-master\data\mod`.
- `out\x64-launch-data\palettes` is a junction to `sorr-vita-master\data\palettes`.
- `out\x64-launch-data\SorR.dat` matches `sorr-vita-master\data\SorR.dat`.

Key hashes:

| File | SHA256 |
| --- | --- |
| `out/x64-launch-data/SorR.dat` | `4B09177B0AE87E72FFF16BE0680793136340D1CF411BE7325F14DEB2C911E65C` |
| `out/x64-launch-data/mod/system.txt` | `EF82CB96B1E7F9C4FF3549AEC03D1C9A406E5E1FF36C1E6B0CC4BB68821F62CC` |
| `out/x64-launch-data/savegame/savegame.sor` | `FB810C72254F6344A248945DFB04B59AF906DFFE4BD4860D06B6418C23D137FF` |
| `out/x64-launch-data/xbox/xbox.cfg` | `4E781D455E3BC9AA841D11CFA6BA7B8397770704E0FF1711A4987C85350592F9` |

Data notes:

- `mod/system.txt` is set to the PC platform selector.
- `savegame/savegame.sor` is the keyboard-working PC save state, updated by x64 proof runs.
- `xbox/xbox.cfg` remains the small unchanged Xbox config file.
- The known-good 32-bit desktop snapshot was not touched.

## Frozen X64 Folder

Created/refreshed:

```text
out/known-good-x64/
```

Included:

- `bgdi.exe`
- `data-overrides/mod/system.txt`
- `data-overrides/savegame/boss1.png`
- `data-overrides/savegame/savegame.sor`
- `data-overrides/savegame/savestate.sor`
- `data-overrides/savegame/trophies.sor`
- `data-overrides/xbox/xbox.cfg`
- `data-overrides/xbox/mod/system.txt`
- `hashes.csv`
- `manifest-summary.json`
- `referenced-game-data-hashes.csv`
- `MANIFEST.md`

The folder intentionally does not duplicate the full game data tree. `referenced-game-data-hashes.csv` records 2414 local referenced game-data files under `sorr-vita-master/data`.

## Keyboard Mapping

Confirmed desktop keyboard defaults:

| Action | Key |
| --- | --- |
| Move up | Arrow Up |
| Move down | Arrow Down |
| Move left | Arrow Left |
| Move right | Arrow Right |
| Special | `X` |
| Attack | `C` |
| Jump | `V` |
| Police | `B` |
| Start / confirm | Enter |
| Back / cancel | Escape or Backspace |

Input diagnostics:

- `reports/x64_all_keys_diag2_stderr.log`
- `reports/x64_arrows_vk_diag_stderr.log`

Observed Bennu key codes:

- Enter/start: `28`
- Special `X`: `45`
- Attack `C`: `46`
- Jump `V`: `47`
- Police `B`: `48`
- Arrow Up/Down/Left/Right: `72`, `80`, `75`, `77`

## Screenshots And Artifacts

Primary playable proof:

- Character select: `reports/x64_playable_char_select.png`
- Live stage gameplay after controls: `reports/x64_stage_after_controls.png`
- Route/stage flow: `reports/x64_stage_route_map.png`

Stability proof:

- 6-minute CSV: `reports/x64_long_stability_6min.csv`
- Minute screenshots: `reports/x64_long_stability_1m.png` through `reports/x64_long_stability_6m.png`

Timing proof:

- x64 actual timing log: `reports/timing_x64_actual_cached_gameplay_stderr.log`
- x64 actual CPU log: `reports/timing_x64_actual_cached_gameplay_cpu.csv`
- x64 timing screenshot: `reports/timing_x64_actual_cached_gameplay_final.png`
- comparison report: `reports/X64_GAMEPLAY_TIMING_NOTES.md`

## Timing Result

Final x64 gameplay:

- Actual gameplay cadence: about 54-55 FPS.
- `waitvsync=0`.
- Explicit `SDL_Delay` resumed after the diagnostic overhead fix.
- No frame skip/jump was active in sampled gameplay.

32-bit Release comparison:

- Actual gameplay cadence: about 55-56 FPS.

Conclusion: x64 gameplay timing now matches the practical known-good 32-bit desktop cadence.

## X64 Fixes Landed

The working x64 runtime includes the following targeted fixes:

- Pointer side table for VM stack cells carrying host pointers.
- File handle table for file sysprocs.
- Render object handle table.
- Palette handle table.
- Palette ID resolution in renderer draw paths.
- Sound/music handle tables.
- x64-safe fixed string buffer consumers such as `MN_STR2A`/`MN_STRACAT`.
- x64 pointer helper consumers for address-producing and address-consuming interpreter opcodes.
- Diagnostic environment-variable caching to remove x64 hot-path diagnostic overhead.

These were kept narrow and proof-driven. No broad VM uintptr rewrite was started.

## Unresolved Issues

- Options submenu parity remains intentionally unresolved.
- iOS app shell has not been built on a real Mac/Xcode environment.
- iOS data layout is not implemented.
- Touch input bridge is not implemented.
- SDL2_mixer and iOS audio codec integration are not implemented.
- Full game-data bundling for iOS is not implemented.
- SOR2-only pruning is intentionally paused.
- x64 proof is gameplay-valid for the tested route, not an exhaustive full-game completion run.

## Stop Point

This phase stops at docs/manifests. No iOS implementation, asset work, Options work, or known-good 32-bit changes were made.
