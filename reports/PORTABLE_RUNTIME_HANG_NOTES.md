# Portable Runtime Hang Notes

## Summary

The post-render nonresponsive window was not a hard runtime hang. The interpreter and renderer continued running after `set_mode`, but the portable desktop build was not pumping SDL/Windows events during startup/intro frames.

Smallest fix applied:

- Added portable-only SDL event pumping in `modules/libsdlhandler/libsdlhandler.c`.
- Enabled it by default only from `sorr-vita-master/cmake/portable/CMakeLists.txt` using `PORTABLE_RUNTIME_PUMP_EVENTS_DEFAULT`.
- Vita `CMakeLists.txt` remains untouched.

Final result:

- `bgdi.exe` builds.
- `SorR.dat` launches.
- Window appears with title `Streets of Rage Remake - v5.2`.
- City scene renders.
- Windows reports `Responding=True` after 25 seconds in the fixed default portable build.
- Menu was not visually reached during this pass, but the exact post-render responsiveness blocker was identified and fixed for the portable desktop build.

Visual proof after fix:

- `reports/hang_fix_visual.window.png`

## Debug Logging Added

Portable-only diagnostic hooks were added behind `PORTABLE_RUNTIME_DIAG`, enabled only in the portable CMake file.

Logging points:

- SDL event pumping: `modules/libsdlhandler/libsdlhandler.c`
- Main interpreter launch flow: `core/bgdi/src/main.c`
- Interpreter loop and first process execution: `core/bgdrtm/src/interpreter.c`
- File opens and search paths: `core/common/files.c`
- Audio initialization and audio stub test: `modules/mod_sound/mod_sound.c`
- `set_mode`, video init, window/renderer/texture creation: `modules/libvideo/g_video.c`
- Frame presents: `modules/librender/g_screen.c`

Runtime toggles:

- `SORR_PORTABLE_DIAG_LOOP=1`
- `SORR_PORTABLE_DIAG_SCRIPT=1`
- `SORR_PORTABLE_DIAG_VIDEO=1`
- `SORR_PORTABLE_DIAG_AUDIO=1`
- `SORR_PORTABLE_DIAG_FILES=1`
- `SORR_PORTABLE_DIAG_EVENTS=1`
- `SORR_PORTABLE_AUDIO_STUB=1`
- `SORR_PORTABLE_PUMP_EVENTS=1`

The portable CMake now also has:

- `PORTABLE_RUNTIME_DIAG=ON`
- `PORTABLE_RUNTIME_PUMP_EVENTS_DEFAULT=ON`

## Build

Build command:

```powershell
$root=(Resolve-Path '.').Path
$env:PATH=(Join-Path $root 'portable-tools\w64devkit\bin') + ';' + (Join-Path $root 'portable-deps\msys2-mingw32\mingw32\bin') + ';' + $env:PATH
$cmake=Join-Path $root 'portable-tools\w64devkit\bin\cmake.exe'
$build=Join-Path $root 'build-portable'
& $cmake --build $build --verbose
```

Build logs:

- Diagnostic build: `reports/portable_hang_build_1.log`
- Fixed default event-pump build: `reports/portable_hang_build_2.log`

Build result:

- `LASTEXITCODE=0`
- Remaining output is legacy warning noise plus the existing TRE CMake deprecation warning.

## Controlled Launch Tests

Each test launched:

```powershell
build-portable\bgdi.exe SorR.dat
```

Working directory:

```powershell
sorr-vita-master\data
```

Observation window:

- 3 seconds initial settle
- 25 seconds measured probe
- Process then stopped manually

### A. Normal Launch

Environment:

- `SORR_PORTABLE_DIAG_LOOP=1`
- `SORR_PORTABLE_DIAG_VIDEO=1`
- `SORR_PORTABLE_DIAG_SCRIPT=1`
- `SORR_PORTABLE_DIAG_AUDIO=1`

Logs:

- `reports/hang_A_normal_diag.stdout.log`
- `reports/hang_A_normal_diag.stderr.log`
- `reports/hang_A_normal_diag.summary.json`
- `reports/hang_A_normal_diag.window.png`

Result:

- Window appears: yes
- Title correct: yes, `Streets of Rage Remake - v5.2`
- Render output: yes, intro/freeware screen captured; renderer continues presenting frames
- CPU: about `4.016` CPU seconds over 25 seconds, about `16.06%` of one core
- Stdout/stderr after `set_mode`: yes, `426` stderr lines after last set-mode related line
- Last key log: `[PORTABLE:VIDEO] frame present count=1000`
- Windows responding: no

Interpretation:

- Not a dead process. The script/render loop continues, but Windows messages are not being pumped.

### B. Audio Disabled/Stubbed

Environment:

- Same as A
- `SORR_PORTABLE_AUDIO_STUB=1`

Logs:

- `reports/hang_B_audio_stub.stdout.log`
- `reports/hang_B_audio_stub.stderr.log`
- `reports/hang_B_audio_stub.summary.json`
- `reports/hang_B_audio_stub.window.png`

Result:

- Window appears: yes
- Title correct: yes
- Render output: yes, intro/freeware screen captured; renderer continues presenting frames
- CPU: about `3.625` CPU seconds over 25 seconds, about `14.5%` of one core
- Stdout/stderr after `set_mode`: yes, `426` stderr lines after last set-mode related line
- Last key log: `[PORTABLE:VIDEO] frame present count=1000`
- Windows responding: no

Interpretation:

- Audio is not the cause. Stubbing audio does not change the nonresponsive flag.

### C. Extra File-Open Logging

Environment:

- `SORR_PORTABLE_DIAG_LOOP=1`
- `SORR_PORTABLE_DIAG_VIDEO=1`
- `SORR_PORTABLE_DIAG_FILES=1`

Logs:

- `reports/hang_C_file_logging.stdout.log`
- `reports/hang_C_file_logging.stderr.log`
- `reports/hang_C_file_logging.summary.json`
- `reports/hang_C_file_logging.window.png`

Result:

- Window appears: yes
- Title correct: yes
- Render output: yes, intro/freeware screen captured; renderer continues presenting frames
- CPU: about `11.562` CPU seconds over 25 seconds, about `46.25%` of one core
- Stdout/stderr after `set_mode`: yes, `2259` stderr lines after last set-mode related line
- File loading continues through many assets; no blocking open was identified
- Windows responding: no

Notable file results:

- Missing optional save/test files:
  - `savegame/savegame.sor`
  - `savegame/testmaker.sor`
- Core assets opened successfully:
  - `SorR.dat`
  - `mod/system.txt`
  - `data/icono.fpg`
  - `data/rudre.wav`
  - `data/english.fpg`
  - many palette, FPG, and WAV assets

Interpretation:

- File I/O is noisy but not the responsiveness blocker.

### D. Extra SDL Event-Loop Logging

Environment:

- `SORR_PORTABLE_DIAG_LOOP=1`
- `SORR_PORTABLE_DIAG_VIDEO=1`
- `SORR_PORTABLE_DIAG_SCRIPT=1`
- `SORR_PORTABLE_DIAG_EVENTS=1`
- `SORR_PORTABLE_PUMP_EVENTS=1`

Logs:

- `reports/hang_D_event_pump.stdout.log`
- `reports/hang_D_event_pump.stderr.log`
- `reports/hang_D_event_pump.summary.json`
- `reports/hang_D_event_pump.window.png`

Result:

- Window appears: yes
- Title correct: yes
- Render output: yes
- CPU: about `3.5` CPU seconds over 25 seconds, about `14%` of one core
- Stdout/stderr after `set_mode`: yes, `466` stderr lines after last set-mode related line
- Last key log: `[PORTABLE:EVENT] SDL_PumpEvents done pending=1 first_type=512`
- Windows responding: yes

Interpretation:

- This isolates the subsystem. Calling `SDL_PumpEvents()` from the SDL handler hook makes Windows report the same running/rendering game as responsive.

## Fixed Default Probe

After enabling portable default event pumping with `PORTABLE_RUNTIME_PUMP_EVENTS_DEFAULT`, I rebuilt and launched with no diagnostic environment variables.

Logs:

- `reports/hang_fix_default.stdout.log`
- `reports/hang_fix_default.stderr.log`
- `reports/hang_fix_default.summary.json`
- `reports/hang_fix_visual.window.png`

Result:

- Window appears: yes
- Title correct: yes, `Streets of Rage Remake - v5.2`
- City scene renders: yes
- Process stayed alive: yes
- Windows responding after 25 seconds: yes
- CPU: about `2.5` CPU seconds over 25 seconds, about `10%` of one core
- Stderr is quiet except existing SDL logs:
  - `INFO: Called set_mode with 320x240x0`
  - `INFO: set_title(Streets of Rage Remake - v5.2)`
  - `INFO: Called set_mode with 320x240x0`

## Diagnosis

The program was not truly hung. The DCB loaded, scripts executed, audio initialized, files continued loading, and the renderer kept presenting frames. The nonresponsive Windows state was caused by missing SDL/Windows message pumping during the active startup/intro loop.

Evidence:

- Normal run kept logging script execution and frame presents up to `frame present count=1000`, but `Responding=False`.
- Audio-stubbed run behaved the same, so audio is not causal.
- File-open run continued opening assets, so file I/O is not the stall point.
- Event-pump run changed only the event pump behavior and became `Responding=True`.
- Fixed default portable build is `Responding=True` with no diagnostic env vars.

Relevant source finding:

- `modules/libsdlhandler/libsdlhandler.c` had `SDL_PumpEvents()` commented out.
- Some individual event consumers also return before draining their queues, but the smallest effective fix was to restore SDL event pumping centrally in the SDL handler hook for portable builds.

## Current Blocker

Post-render Windows nonresponsiveness is fixed in the portable desktop build.

The next blocker is no longer the Windows nonresponsive flag. The next phase should be menu/input progression under the now-responsive desktop runtime, still before any iOS work.
