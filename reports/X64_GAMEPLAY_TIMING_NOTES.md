# X64 Gameplay Timing Notes

## Scope

Goal: compare the known-good 32-bit desktop timing path against the x64 runtime and fix the x64-only gameplay slowdown without touching the known-good 32-bit snapshot, iOS work, asset trimming, or the Options submenu.

The symptom was smooth but slow gameplay. Menus and audio felt normal, so this was treated as a frame pacing or timer problem first rather than a renderer throughput issue.

## Files Instrumented

- `sorr-vita-master/modules/librender/g_frame.c`
  - Added `SORR_PORTABLE_DIAG_TIMING` frame summaries around `gr_wait_frame`.
  - Logs target FPS, actual FPS, `frame_ms`, `frame_time`, `ticks_per_frame`, `fps_partial`, `SDL_Delay` count/total, frame skip/jump state, `waitvsync`, global FPS, and speed gauge.
- `sorr-vita-master/modules/librender/g_screen.c`
  - Added present summaries around `SDL_RenderPresent`.
- `sorr-vita-master/modules/libvideo/g_video.c`
  - Inspected for video init and mode behavior. No timing fix was needed there.
- `sorr-vita-master/modules/mod_video/mod_video.c`
  - Added `SET_FPS` parameter logging.
- `sorr-vita-master/modules/mod_timers/mod_timers.c`
  - Added once-per-second timer summaries for `timer[0..9]`.
- `sorr-vita-master/core/bgdrtm/src/interpreter.c`
  - Added once-per-second `MN_FRAME` aggregate logging.
- `sorr-vita-master/cmake/portable/portable_diag.h`
  - Added the `TIMING` diagnostic category.
  - Added cached environment-variable lookups for portable diagnostics.

## Timing Build Setup

Disposable timing builds were used so the frozen 32-bit proof remained untouched:

```powershell
cmake -S sorr-vita-master -B build-portable-timing-32 -G "MinGW Makefiles" -DCMAKE_BUILD_TYPE=Release -DCMAKE_TOOLCHAIN_FILE=sorr-vita-master/cmake/portable/toolchains/mingw32.cmake
cmake --build build-portable-timing-32

cmake -S sorr-vita-master -B build-portable-timing-x64 -G "MinGW Makefiles" -DCMAKE_BUILD_TYPE=Release -DCMAKE_TOOLCHAIN_FILE=sorr-vita-master/cmake/portable/toolchains/mingw64.cmake
cmake --build build-portable-timing-x64
```

Timing data folders:

- `out/timing-32-data`
- `out/timing-x64-data`

Actual x64 runtime was also rebuilt after the fix:

```powershell
$env:PATH="$PWD\portable-tools\w64devkit-x64\bin;$PWD\portable-deps\msys2-mingw64\mingw64\bin;$env:PATH"
portable-tools\w64devkit-x64\bin\cmake.exe --build build-portable-x64 --verbose
```

Final actual x64 executable:

- `build-portable-x64/bgdi.exe`
- SHA256: `4FE38BC20771428C6C986E72A09441DB9DE246170A3B009EE61528C413729592`

## Launch Method

All gameplay timing probes launched from prepared data folders with timing diagnostics enabled:

```powershell
$env:SORR_PORTABLE_DIAG_TIMING="1"
cd out/x64-launch-data
..\..\build-portable-x64\bgdi.exe SorR.dat
```

The same stage/gameplay path was reached by scripted keyboard input where possible, then sampled during live gameplay.

## Results

### Menu / Intro Baseline

No-input idle/menu probes showed both 32-bit and x64 running near the expected frame cadence before gameplay:

- 32-bit idle: target FPS 60, actual/present around 55-56 FPS.
- x64 idle: target FPS 60, actual/present around 55-56 FPS.
- `waitvsync=0` in both.

Artifacts:

- `reports/timing_32_idle_stderr.log`
- `reports/timing_x64_idle_stderr.log`

### 32-bit Release Gameplay Baseline

Known-good 32-bit timing build during live gameplay:

- target FPS: 60
- actual FPS: about 54.78-56.16
- present FPS: about 55.28-56.27
- `waitvsync=0`
- `SDL_Delay`: about 51-53 calls/sec
- delay total: about 674-735 ms/sec
- skip/jump: 0
- `frame_time`: about 0.002-0.004
- `MN_FRAME`: about 5263-6210/sec in the sampled gameplay tail
- CPU sample tail: about 22.3 CPU seconds at 142 elapsed seconds

Artifacts:

- `reports/timing_32_release_gameplay_stderr.log`
- `reports/timing_32_release_gameplay_cpu.csv`
- `reports/timing_32_release_gameplay_final.png`

### X64 Release Gameplay Before Fix

The first optimized x64 gameplay probe reproduced the slow gameplay:

- target FPS: 60
- actual FPS: about 18.76-22.35
- present FPS: about 18.59-22.33
- `waitvsync=0`
- `SDL_Delay`: 0 calls/sec
- delay total: 0 ms/sec
- skip/jump: 0
- `frame_time`: about 0.037-0.065
- CPU sample tail: about 107 CPU seconds at 142 elapsed seconds

This ruled out double waiting. The runtime was not delayed by `SDL_Delay` or vsync; it was already taking longer than the frame budget before the wait path could sleep.

Artifacts:

- `reports/timing_x64_release_gameplay_stderr.log`
- `reports/timing_x64_release_gameplay_cpu.csv`
- `reports/timing_x64_release_gameplay_final.png`

### X64 Timing Build After Fix

After caching portable diagnostic environment checks:

- actual FPS: about 53.47-55.83
- present FPS: about 52.89-55.61
- target FPS: 60
- `waitvsync=0`
- `SDL_Delay`: about 50-52 calls/sec
- delay total: about 602-670 ms/sec
- skip/jump: 0
- CPU sample tail: about 37 CPU seconds at 142 elapsed seconds

Artifacts:

- `reports/timing_x64_cached_gameplay_stderr.log`
- `reports/timing_x64_cached_gameplay_cpu.csv`
- `reports/timing_x64_cached_gameplay_final.png`

### Actual X64 Runtime After Fix

The real target `build-portable-x64/bgdi.exe` was rebuilt and tested from `out/x64-launch-data`.

Result:

- gameplay reached
- window title remained `Streets of Rage Remake - v5.2`
- actual FPS: about 54.00-55.23
- present FPS: about 54.40-55.28
- target FPS: 60
- `waitvsync=0`
- `SDL_Delay`: about 51-53 calls/sec
- delay total: about 599-628 ms/sec
- skip/jump: 0
- `frame_time`: about 0.004-0.006
- CPU sample tail: about 38.5 CPU seconds at 142 elapsed seconds

Artifacts:

- `reports/timing_x64_actual_cached_gameplay_stderr.log`
- `reports/timing_x64_actual_cached_gameplay_cpu.csv`
- `reports/timing_x64_actual_cached_gameplay_final.png`

## Findings

The x64 slowdown was not caused by:

- vsync double throttling: `waitvsync=0` in all relevant runs.
- explicit delay double throttling: the bad x64 run had `delay_count=0`.
- wrong target FPS: both 32-bit and x64 logged the same `SET_FPS` sequence, including startup `SET_FPS 24,0` and gameplay/menu `SET_FPS 60,0`.
- frame skip activation: skip/jump stayed 0 in the sampled gameplay runs.
- timer unit drift: timer summaries advanced consistently, and the x64 failure had high per-frame processing time rather than a bad sleep duration.

The confirmed cause was diagnostic overhead on x64 hot paths introduced while stabilizing x64 pointer and native-handle behavior.

The portable runtime is built with `PORTABLE_RUNTIME_DIAG`, and x64 gameplay hits several diagnostic guards frequently, including pointer-stack, render-handle, palette-handle, and input paths. Before the fix, even disabled categories repeatedly called `getenv()` and string-category checks from hot gameplay paths. The 32-bit build did not hit the same x64-only diagnostic paths, which is why 32-bit gameplay and menus remained normal.

## Fix

`sorr-vita-master/cmake/portable/portable_diag.h` now caches environment-variable checks in-process:

- first lookup reads `getenv`
- later checks reuse the cached enabled/disabled value
- added explicit category gates for `INPUT`, `PALETTE`, `RENDER`, and `TIMING`

This keeps diagnostic behavior configurable at process startup while removing repeated hot-path environment lookups. Runtime behavior is otherwise unchanged.

Why this is x64-safe:

- it does not alter VM stack layout
- it does not alter pointer side-table semantics
- it does not alter file/native handle tables
- it does not alter frame pacing logic
- it only changes diagnostic-gate overhead

Why 32-bit behavior is preserved:

- diagnostics remain disabled unless environment variables are set
- 32-bit data/runtime files are not modified
- the known-good 32-bit snapshot was not touched or rebuilt
- cached diagnostic flags have the same effective value for this runtime because diagnostics are configured before launch

## Current Status

The x64 gameplay timing issue is fixed for the tested path. The actual x64 runtime now matches the practical 32-bit gameplay cadence:

- 32-bit Release live gameplay: about 55-56 FPS
- x64 actual runtime after fix: about 54-55 FPS

No iOS work, asset trimming, Options submenu work, or broad pointer rewrite was done in this phase.

## Remaining Notes

- The timing instrumentation is intentionally diagnostic-only and gated by `SORR_PORTABLE_DIAG_TIMING`.
- Diagnostic environment variables are now effectively process-start settings. Changing them after launch is not expected to take effect.
- An earlier x64 timing run without `CMAKE_BUILD_TYPE=Release` showed about 19-20 FPS, but that was an unoptimized timing-build artifact and was not used as the final comparison.
