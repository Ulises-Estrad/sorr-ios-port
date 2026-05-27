# x64 Runtime Stabilization

## Goal

Stabilize only the x64 interpreter path enough to move beyond the systemic pointer truncation crash. Do not modify the known-good 32-bit desktop build, do not resume iOS, and do not patch unrelated modules.

## Build Target

- Build directory: `build-portable-x64`
- Launch data: `out/x64-launch-data`
- Runtime: `build-portable-x64/bgdi.exe`
- Data: `out/x64-launch-data/SorR.dat`

## Build Command

```powershell
$root=(Get-Location).Path
$env:PATH=(Join-Path $root 'portable-tools\w64devkit-x64\bin') + ';' + (Join-Path $root 'portable-deps\msys2-mingw64\mingw64\bin') + ';' + $env:PATH
$cmake=Join-Path $root 'portable-tools\w64devkit-x64\bin\cmake.exe'
& $cmake --build build-portable-x64 --verbose
```

Latest clean build:

- `reports/x64_runtime_stabilization_build_8_clean.log`
- Result: `BUILD8_EXIT=0`

## Strategy Chosen

Implemented an x64-only pointer side table in `sorr-vita-master/core/bgdrtm/src/interpreter.c`.

The VM stack remains `int`-based for DCB and 32-bit compatibility. When an x64 opcode pushes a host address, the low 32 bits are still written into the VM stack cell, and the full `uintptr_t` is stored in a parallel table keyed by the stack cell address plus the low value. Consumers that need a host pointer recover the full pointer through the helper and fall back to the low 32-bit value only when no table entry exists.

Helpers added:

- `portable_x64_stack_set_ptr`
- `portable_x64_stack_get_ptr`
- `portable_x64_stack_peek_ptr`
- `portable_x64_stack_copy_cell`
- `portable_x64_stack_adjust_ptr`

This is intentionally short-term and x64-only. It does not solve the broader external module ABI issue where module functions accept `int *params`.

## Address Producers Identified

Address-producing interpreter opcodes:

- `MN_PRIVATE`, including unsigned/word/byte/string/float variants
- `MN_PUBLIC`, including unsigned/word/byte/string/float variants
- `MN_LOCAL`, including unsigned/word/byte/string/float variants
- `MN_GLOBAL`, including unsigned/word/byte/string/float variants
- `MN_REMOTE`, including unsigned/word/byte/string/float variants
- `MN_REMOTE_PUBLIC`, including unsigned/word/byte/string/float variants
- `MN_INDEX`, which offsets an existing address stack cell
- `MN_ARRAY`, which offsets an existing address stack cell by an indexed stride
- `MN_DUP`, when duplicating a pointer-bearing stack cell

Producer coverage implemented in this pass:

- `MN_GLOBAL` first, for the known crash chain.
- Then `MN_PRIVATE`, `MN_PUBLIC`, `MN_LOCAL`, `MN_REMOTE`, and `MN_REMOTE_PUBLIC`.
- `MN_DUP`, `MN_INDEX`, and `MN_ARRAY` now preserve or adjust side-table pointer entries.

## Pointer Consumers Identified

Pointer-consuming interpreter paths:

- `MN_LETNP`, `MN_LET`
- `MN_INC`, `MN_DEC`, `MN_POSTINC`, `MN_POSTDEC`
- `MN_VARADD`, `MN_VARSUB`, `MN_VARMUL`, `MN_VARDIV`, `MN_VARMOD`
- `MN_VAROR`, `MN_VARXOR`, `MN_VARAND`, `MN_VARROR`, `MN_VARROL`
- string assignment: `MN_LETNP | MN_STRING`, `MN_LET | MN_STRING`
- `MN_VARADD | MN_STRING`
- pointer reads: `MN_PTR`, `MN_STRING | MN_PTR`, `MN_WORD | MN_PTR`, `MN_BYTE | MN_PTR`
- fixed-string conversion: `MN_A2STR`, `MN_STR2A`, `MN_STRACAT`
- `MN_POINTER2STR`
- sysproc/syscall pointer parameters, especially param types containing `P`, `V`, or `+`

Consumer coverage implemented in this pass:

- `MN_LETNP` and `MN_LET` for DWORD values.
- DWORD direct operations: inc/dec/postinc/postdec and var math/bitwise write-through operations.
- string `LETNP` and `LET`.
- pointer reads for DWORD/string/word/byte `MN_PTR` variants.
- `MN_A2STR`.

Not yet fully covered:

- Word/byte/float direct assignment and var math write-through groups.
- `MN_VARADD | MN_STRING`.
- `MN_STR2A`, `MN_STRACAT`, and `MN_POINTER2STR`.
- General module ABI repair for pointer params.

## Sysproc Bridges Added

Because external module functions still receive `int *params`, full pointers cannot be passed generically without an ABI change. For confirmed early boot blockers, interpreter-local x64 bridges were added:

- `GET_DESKTOP_SIZE(PP)`: resolves both pointer params and writes desktop width/height using `GetSystemMetrics`.
- `LOAD(SV++)` and `SAVE(SV++)`: resolves data/type-definition pointers from the side table and calls `loadtypes`/`savetypes` through the same core file helpers used by `mod_file`.

This is a tactical bridge, not the final ABI design.

## Run History

### Run 1

Log: `reports/x64_runtime_stabilization_run_1_stderr_clean.log`

Result:

- `MN_GLOBAL -> MN_LETNP` no longer crashed.
- Next crash was `MN_SYSPROC GET_DESKTOP_SIZE(PP)` because pointer params were truncated before reaching `mod_wm`.

### Run 2

Log: `reports/x64_runtime_stabilization_run_2_stderr_clean.log`

Result:

- Confirmed sysproc: `code=296 name=GET_DESKTOP_SIZE params=2 types=PP`.

### Run 3

Log: `reports/x64_runtime_stabilization_run_3_stderr_clean.log`

Result:

- `GET_DESKTOP_SIZE` bridge worked.
- Next crash chain: `MN_GLOBAL | MN_STRING -> MN_LETNP | MN_STRING`.

### Run 4

Log: `reports/x64_runtime_stabilization_run_4_stderr_clean.log`

Result:

- String `LETNP`/`LET` bridge worked.
- Next crash chain: `MN_SYSPROC LOAD(SV++)`.

### Run 5

Log: `reports/x64_runtime_stabilization_run_5_stderr_clean.log`

Result:

- `LOAD(SV++)` bridge worked.
- Savegame and trophy files loaded:
  - `savegame/savegame.sor`, result `1764`
  - `savegame/trophies.sor`, result `112`
- Next crash chain: `MN_PRIVATE -> MN_LET`.

### Run 6

Log: `reports/x64_runtime_stabilization_run_6_stderr_clean.log`

Result:

- `MN_PRIVATE -> MN_LET` worked.
- Next crash chain: `MN_PTR`.

### Run 7

Log: `reports/x64_runtime_stabilization_run_7_stderr_clean.log`

Result:

- Pointer-read `MN_PTR` worked.
- Next crash chain: `MN_POSTINC`.

### Run 8

Log: `reports/x64_runtime_stabilization_run_8_stderr_clean.log`

Result:

- DWORD `MN_POSTINC` and surrounding direct write-through group worked.
- With step and pointer logging enabled, the process did not crash before the 25-second timeout; it was still progressing through `CARGAR_PARTIDA`.

### Run 9

Log: `reports/x64_runtime_stabilization_run_9_stderr_clean.log`

Result:

- Low-noise run reached `MN_FRAME proc=CARGAR_PARTIDA`.
- It also reached several `MN_A2STR` conversions backed by full x64 pointers.
- The non-debugger process then exited shortly after the first frame; no backtrace was available from this run.

### Run 10

Log: `reports/x64_runtime_stabilization_run_10_stderr_clean.log`

Result:

- Step-only run did not crash before timeout.
- It was still progressing through `CARGAR_PARTIDA`; per-opcode logging slowed execution too much to reach the later post-frame point.

### Run 12 under gdb

Log: `reports/x64_runtime_stabilization_run_12_gdb_clean.log`

Result:

- The runtime stayed alive until the tool timeout.
- Reached title update:
  - `set_title(Streets of Rage Remake - v5.2)`
- Reached later video mode:
  - `Called set_mode with 416x240x0`
- No access-violation backtrace was produced before the timeout.

### Run 13

Log: `reports/x64_runtime_stabilization_run_13_stderr_clean.log`

Result:

- Video-only non-debugger run still logs first frame present.
- Process exits after first frame with no useful exit code captured by `Start-Process`.

### Visible PowerShell Launch

Command shape:

```powershell
$env:PATH='portable-tools\w64devkit-x64\bin;portable-deps\msys2-mingw64\mingw64\bin;' + $env:PATH
cd out\x64-launch-data
..\..\build-portable-x64\bgdi.exe SorR.dat
```

Result:

- A window appeared.
- The window title observed before crash was `SorR.dat`.
- It did not stay open.
- It did not reach `Streets of Rage Remake - v5.2` in this visible direct launch.
- The visible console printed: `bgdi exited with code -1073741819`.
- `-1073741819` is `0xC0000005` / access violation.
- No crash dialog was observed in the captured screen.

Screenshot:

- `reports/x64_visible_startprocess_probe.png`

### Visible Start-Process Probe

Command shape:

```powershell
$env:PATH='portable-tools\w64devkit-x64\bin;portable-deps\msys2-mingw64\mingw64\bin;' + $env:PATH
Start-Process -FilePath build-portable-x64\bgdi.exe -ArgumentList 'SorR.dat' -WorkingDirectory out\x64-launch-data -PassThru
```

Important: this probe used no `-RedirectStandardOutput`, no `-RedirectStandardError`, no `-WindowStyle Hidden`, and no `-NoNewWindow`.

Artifacts:

- Poll log: `reports/x64_visible_startprocess_probe.csv`
- Screenshot: `reports/x64_visible_startprocess_probe.png`

Result:

- Process existed at launch and was responsive.
- At about 3.7 seconds, `MainWindowTitle` was `SorR.dat`, `Responding=True`, and a nonzero window handle was present.
- It exited before 4.2 seconds.
- Exact exit code: `-1073741819` / `0xC0000005`.

Event Viewer:

- Provider: `Application Error`
- Event ID: `1000`
- Exception code: `0xc0000005`
- Faulting application: `bgdi.exe`
- Faulting module: `bgdi.exe`
- Fault offset: `0x000000000000c2bc`
- Example report ID: `d4e36b79-cf99-4b7d-8cfe-773d554144e7`

Offset resolution:

```powershell
portable-tools\w64devkit-x64\bin\addr2line.exe -f -C -e build-portable-x64\bgdi.exe 0x14000c2bc
```

Resolved to:

- Function: `file_gets`
- Source: `sorr-vita-master/core/common/files.c:301`

Interpretation:

- The visible launch method is not the reason for the crash.
- Hidden/redirected probes were not falsely creating the access violation.
- The current x64 blocker appears to be a file-handle pointer truncation path. `file_gets` crashes immediately dereferencing its first argument, so a Bennu file handle returned/stored as a 32-bit integer is likely being consumed later as a native `file *`.

## Current Status

Fixed:

- Original systemic x64 crash chain from `MN_GLOBAL` pointer truncation into `MN_LETNP`.
- Subsequent confirmed pointer-truncation crashes in:
  - `GET_DESKTOP_SIZE(PP)`
  - `LOAD(SV++)`
  - string `LETNP`
  - `MN_PRIVATE -> MN_LET`
  - `MN_PTR`
  - `MN_POSTINC`

Evidence of progress:

- x64 reaches first render/frame.
- x64 loads `savegame/savegame.sor` and `savegame/trophies.sor`.
- x64 reaches `MN_FRAME` in `CARGAR_PARTIDA`.
- Under gdb, x64 reaches `set_title(Streets of Rage Remake - v5.2)` and a later `416x240` mode change.

Current blocker:

- Visible launch exits before reaching title/menu.
- Exact visible exit code is `0xC0000005`.
- Windows Event Viewer records the fault at `bgdi.exe+0xc2bc`.
- `addr2line` maps the crash to `file_gets` in `sorr-vita-master/core/common/files.c:301`.
- This points to the next x64 class: file handles/native pointers returned through 32-bit Bennu values, likely `FOPEN`/`FGETS` or related file APIs.

Recommended next step:

1. Do not add broad pointer fixes yet.
2. Instrument the file sysproc/syscall path to identify the exact script operation before `file_gets`.
3. Apply the same side-table strategy to file-handle producers/consumers if confirmed:
   - producer candidate: `FOPEN`
   - consumer candidates: `FGETS`, `FCLOSE`, `FREAD`, `FWRITE`, `FSEEK`, `FTELL`, `FLENGTH`, `FEOF`
4. Keep this scoped to x64 runtime behavior unless/until the module ABI is deliberately widened.

## Guardrails

- Known-good 32-bit desktop data/build was not rebuilt or launched.
- Vita CMake remains untouched.
- iOS work remains paused.
- No asset trimming was done.

## Autonomous Stabilization Pass - 2026-05-27

This pass continued from the confirmed `file_gets` x64 crash and kept the known-good 32-bit desktop snapshot untouched. All fixes below are narrow x64/native-handle bridges or x64 pointer-stack consumers; they do not silence warnings as a substitute for a fix.

Build command used throughout:

```powershell
$env:PATH=(Join-Path $PWD 'portable-tools\w64devkit-x64\bin') + ';' + (Join-Path $PWD 'portable-deps\msys2-mingw64\mingw64\bin') + ';' + $env:PATH
portable-tools\w64devkit-x64\bin\cmake.exe --build build-portable-x64 --verbose
```

Launch command shape used for visible/control probes:

```powershell
Start-Process -FilePath build-portable-x64\bgdi.exe -ArgumentList 'SorR.dat' -WorkingDirectory out\x64-launch-data -PassThru
```

### Iteration 1 - file handles

- Crash signature: `0xC0000005`, Event Viewer `bgdi.exe+0xc2bc`.
- Backtrace/source: `file_gets()` at `sorr-vita-master/core/common/files.c:301`.
- Class: native object handle truncation.
- Root cause: `mod_file` returned native `file *` through a 32-bit Bennu integer, then consumers cast the truncated value back to `file *`.
- Files changed: `sorr-vita-master/modules/mod_file/mod_file.c`.
- Fix: added `_WIN64` file-handle table and resolved handles in `FOPEN`, `FCLOSE`, `FGETS`, `FREAD`, `FWRITE`, `FREADC`, `FWRITEC`, `FSEEK`, `FREWIND`, `FTELL`, `FFLUSH`, `FLENGTH`, `FPUTS`, and `FEOF`.
- x64 safety: x64 returns small stable handles to Bennu code and resolves to native pointers only inside the module.
- 32-bit preservation: non-Win64 macros keep the original pointer-as-int behavior.
- Result: x64 passed the `file_gets` crash.
- Next blocker: render object pointer crash.

### Iteration 2 - render objects

- Crash signature: `0xC0000005`, Event Viewer `bgdi.exe+0x159f2`.
- Backtrace/source: `gr_destroy_object()` at `sorr-vita-master/modules/librender/g_object.c:207`.
- Class: native object handle truncation.
- Root cause: render object pointers were returned/stored as 32-bit integer object IDs on x64.
- Files changed: `sorr-vita-master/modules/librender/g_object.c`.
- Fix: added `_WIN64` render object handle table for `gr_new_object()` and `gr_destroy_object()`.
- x64 safety: Bennu-facing IDs stay small; renderer resolves them internally.
- 32-bit preservation: non-Win64 path still returns/casts native pointers as before.
- Result: x64 passed the render-object destroy crash.
- Next blocker: cached `GRAPHPTR` truncation.

### Iteration 3 - cached graph pointer

- Crash signature: `0xC0000005`, Event Viewer `bgdi.exe+0x2d1d2`.
- Backtrace/source: `gr_blit()` at `sorr-vita-master/modules/libblit/g_blit.c:2541`.
- Class: VM/local native pointer truncation.
- Root cause: `GRAPHPTR` cached a native `GRAPH *` in a 32-bit local field.
- Files changed: `sorr-vita-master/modules/librender/g_instance.c`.
- Fix: on `_WIN64`, `draw_instance()` recomputes the graph with `instance_graph(i)` and `draw_instance_info()` stores only a non-pointer sentinel in `GRAPHPTR`.
- x64 safety: avoids persisting a host pointer in the VM local field.
- 32-bit preservation: non-Win64 keeps the original `GRAPHPTR` cache.
- Result: x64 passed the `gr_blit` null/truncated data crash.
- Next blocker: palette pointer/handle crashes.

### Iteration 4 - palette sysproc handles and raw pointer params

- Crash signatures:
  - `pal_get()` at `sorr-vita-master/modules/libgrbase/g_pal.c:540`.
  - `pal_set()` at `sorr-vita-master/modules/libgrbase/g_pal.c:562`.
  - `modmap_get_point()` at `sorr-vita-master/modules/mod_map/mod_map.c:264`.
- Class: native object handle truncation and VM stack pointer/address issue.
- Root cause: palette IDs and output pointer parameters were passed through 32-bit Bennu values.
- Files changed: `sorr-vita-master/modules/mod_map/mod_map.c`, `sorr-vita-master/core/bgdrtm/src/interpreter.c`.
- Fix: added `_WIN64` palette handle table in `mod_map`; added exported `portable_x64_sysproc_pointer_param()` to resolve `P` parameters from the x64 stack pointer side table; patched palette sysprocs and `POINT_GET`.
- x64 safety: handles remain small at the script boundary; raw output pointers are resolved from the existing stack side table.
- 32-bit preservation: non-Win64 macros retain old casts.
- Result: x64 passed palette get/set and point-output crashes.
- Next blocker: SDL_mixer crash from sound/music handle truncation.

### Iteration 5 - sound and music handles

- Crash signature: `0xC0000005`, faulting module `SDL2_mixer.dll`, offset `0x15c80`.
- Backtrace/source: external mixer crash while playing/using a sound object.
- Class: native object handle truncation.
- Root cause: `Mix_Chunk *` and `Mix_Music *` were returned through 32-bit Bennu integers and later passed to SDL_mixer.
- Files changed: `sorr-vita-master/modules/mod_sound/mod_sound.c`.
- Fix: added `_WIN64` tables for sound chunk and music handles; patched `LOAD_WAV`, `PLAY_WAV`, `UNLOAD_WAV`, `SET_WAV_VOLUME`, `LOAD_MUSIC`, `PLAY_MUSIC`, `FADE_MUSIC_IN`, and `UNLOAD_MUSIC`.
- x64 safety: SDL_mixer receives real native pointers resolved inside `mod_sound`.
- 32-bit preservation: non-Win64 macros retain original pointer casts.
- Result: x64 passed the SDL_mixer crash.
- Next blocker: interpreter float pointer consumers.

### Iteration 6 - float VM pointer consumers

- Crash signatures:
  - `instance_go()` at `sorr-vita-master/core/bgdrtm/src/interpreter.c:2386` (`MN_FLOAT | MN_LETNP`).
  - `instance_go()` at `sorr-vita-master/core/bgdrtm/src/interpreter.c:2428` (`MN_FLOAT | MN_VARSUB`).
- Class: VM stack pointer/address issue.
- Root cause: float direct-variable operations dereferenced pointer-carrying stack cells as raw 32-bit values.
- Files changed: `sorr-vita-master/core/bgdrtm/src/interpreter.c`.
- Fix: patched `_WIN64` `MN_FLOAT | MN_LETNP`, `MN_FLOAT | MN_LET`, and the float `VARADD/VARSUB/VARMUL/VARDIV` family to resolve destination addresses with `portable_x64_stack_get_ptr()`.
- x64 safety: only pointer-consuming float operations use the side table; numeric stack values stay 32-bit.
- 32-bit preservation: old expressions remain under `#else`.
- Result: x64 stayed alive and responsive for 30 seconds with the final window title `Streets of Rage Remake - v5.2`.
- Next blocker: fixed-length string buffer pointer crash.

### Iteration 7 - fixed-length string buffers

- Crash signature: `0xC0000005` in `msvcrt!strncpy`.
- Backtrace/source: `instance_go()` at `sorr-vita-master/core/bgdrtm/src/interpreter.c:1884` (`MN_STR2A`).
- Class: string pointer issue.
- Root cause: `MN_STR2A` treated `stack[-2]` as a `char *` by reading only the 32-bit stack cell.
- Files changed: `sorr-vita-master/core/bgdrtm/src/interpreter.c`.
- Fix: patched `_WIN64` `MN_STR2A` and sibling `MN_STRACAT` to resolve the fixed string buffer pointer through `portable_x64_stack_get_ptr()`.
- x64 safety: only the destination buffer pointer is widened through the existing side table.
- 32-bit preservation: old casts remain under `#else`.
- Result: x64 passed the `strncpy` crash.
- Next blocker: blit crash from palette handle stored in instance `PALETTEID`.

### Iteration 8 - renderer `PALETTEID`

- Crash signature: `0xC0000005`, Event Viewer `bgdi.exe+0x28dc6`.
- Backtrace/source: `draw_span_8to16_translucent()` at `sorr-vita-master/modules/libblit/g_blit.c:431`.
- gdb evidence: `dest`, `orig`, `ghost1`, and `ghost2` were sane; `pcolorequiv` was `0x301`, matching a small palette handle plus the `PALETTE.colorequiv` offset rather than a host pointer.
- Class: native object handle truncation.
- Root cause: `librender/g_instance.c` cast `LOCDWORD(..., PALETTEID)` directly to `PALETTE *`, but x64 palette IDs are now small handles.
- Files changed: `sorr-vita-master/modules/mod_map/mod_map.c`, `sorr-vita-master/modules/librender/g_instance.c`.
- Fix: exported `modmap_x64_palette_from_handle()` from the x64 palette table and resolved `PALETTEID` before the temporary `map->format->palette` override in both instance draw paths.
- x64 safety: renderer gets the real `PALETTE *` from the handle table before blitting.
- 32-bit preservation: non-Win64 macro still casts the value directly to `PALETTE *`.
- Result: x64 passed the late intro/demo blit crash.
- Next blocker: none observed in runtime/render path.

## Final x64 Proof

Build:

- Last x64 build succeeded with `BUILD_EXIT=0`.
- Build log: `reports/x64_iter_paletteid_build.log`.

Stability:

- `reports/x64_paletteid_long_run.csv`: x64 stayed alive and responsive for 180 seconds after the palette fix.
- `reports/x64_long_stability_6min.csv`: x64 stayed alive and responsive for 6 minutes, cycling story and attract gameplay.

Screenshots:

- City/title startup render: `reports/x64_alive_15s_screen.png`.
- Attract gameplay after former crash point: `reports/x64_paletteid_run_180s.png`.
- Six-minute stability samples: `reports/x64_long_stability_1m.png` through `reports/x64_long_stability_6m.png`.
- Character select reached from early Start: `reports/x64_playable_char_select.png`.
- Route dialogue/stage setup reached: `reports/x64_stage_route_map.png`.
- Live stage gameplay reached as Axel: `reports/x64_stage_after_controls.png`.

Input proof:

- Early Enter/Start reaches `SELECT PLAYER`.
- Enter/C confirmation advances from character select to route/stage flow.
- `PORTABLE_INPUT_DIAG=1` confirms Bennu keyboard state changes for:
  - Enter/start: code `28`.
  - Attack `C`: code `46`.
  - Jump `V`: code `47`.
  - Special `X`: code `45`.
  - Police `B`: code `48`.
  - Up/Down/Left/Right: codes `72`, `80`, `75`, `77` via virtual-key injection.
- Logs:
  - `reports/x64_all_keys_diag2_stderr.log`
  - `reports/x64_arrows_vk_diag_stderr.log`

Current status:

- x64 boots `SorR.dat`.
- x64 renders and stays responsive.
- x64 reaches character select.
- x64 reaches route/stage sequence.
- x64 reaches live gameplay as Axel.
- Keyboard defaults are visible to the runtime through `mod_key` diagnostics.
- No new access violation was observed after the renderer `PALETTEID` fix.

Known caveats:

- The x64 launch copy under `out/x64-launch-data` updated its local `savegame/savegame.sor` during proof runs. The frozen 32-bit desktop snapshot and `sorr-vita-master/data` were not rebuilt or launched.
- Synthetic input at the attract-demo `PRESS START` overlay does not interrupt that demo, but early Start from the startup city scene correctly reaches character select.
- Options submenu parity remains intentionally out of scope.
