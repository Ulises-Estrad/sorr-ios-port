# Portable Runtime Build Notes

## Status

Desktop portable runtime proof is successful through DCB load and first window render.

- Original Vita `CMakeLists.txt`: untouched. Verified current `sorr-vita-master/CMakeLists.txt` still matches `sorr-vita.zip` entry `sorr-vita-master/CMakeLists.txt`.
- New portable build file: `sorr-vita-master/cmake/portable/CMakeLists.txt`.
- Built executable: `build-portable/bgdi.exe`.
- Data used: `sorr-vita-master/data/SorR.dat`.
- Runtime reaches `main()`: yes.
- Runtime opens `SorR.dat`: yes. After renaming the output executable to `bgdi.exe`, the DCB compatibility error disappeared and the runtime advanced into the game script.
- Runtime renders a window: yes. Screenshot captured at `reports/portable_launch_window_4.png`.
- Window title: `Streets of Rage Remake - v5.2`.
- Remaining blocker for fully playable desktop proof: the SDL window is created and rendered, but Windows reports the process as nonresponsive after 25 seconds. This should be treated as the next runtime/debugging blocker before moving toward iOS.

## Build Command Used

Configure:

```powershell
$root=(Resolve-Path '.').Path
$env:PATH=(Join-Path $root 'portable-tools\w64devkit\bin') + ';' + (Join-Path $root 'portable-deps\msys2-mingw32\mingw32\bin') + ';' + $env:PATH
$cmake=Join-Path $root 'portable-tools\w64devkit\bin\cmake.exe'
$src=Join-Path $root 'sorr-vita-master\cmake\portable'
$build=Join-Path $root 'build-portable'
$prefix=Join-Path $root 'portable-deps\msys2-mingw32\mingw32'
$cc=Join-Path $root 'portable-tools\w64devkit\bin\gcc.exe'
& $cmake -S $src -B $build -G Ninja -DCMAKE_BUILD_TYPE=RelWithDebInfo "-DCMAKE_C_COMPILER=$cc" "-DCMAKE_PREFIX_PATH=$prefix" "-DCMAKE_POLICY_VERSION_MINIMUM=3.5"
```

Build:

```powershell
$root=(Resolve-Path '.').Path
$env:PATH=(Join-Path $root 'portable-tools\w64devkit\bin') + ';' + (Join-Path $root 'portable-deps\msys2-mingw32\mingw32\bin') + ';' + $env:PATH
$cmake=Join-Path $root 'portable-tools\w64devkit\bin\cmake.exe'
$build=Join-Path $root 'build-portable'
& $cmake --build $build --verbose
```

Final build log:

- `reports/portable_build_6.log`
- Result: `LASTEXITCODE=0`

## Dependency Setup

Toolchain:

- `portable-tools/w64devkit`
- 32-bit GCC was used intentionally because this BennuGD code stores/casts pointers through `int` in several runtime/module paths.

Extracted MSYS2 MinGW32 packages under `portable-deps/msys2-mingw32`:

- `mingw-w64-i686-zlib-1.3.2-2`
- `mingw-w64-i686-libpng-1.6.58-1`
- `mingw-w64-i686-SDL2-2.32.10-1`
- `mingw-w64-i686-SDL2_mixer-2.8.2-1`
- `mingw-w64-i686-flac-1.5.0-1`
- `mingw-w64-i686-mpg123-1.33.5-1`
- `mingw-w64-i686-opusfile-0.12-4`
- `mingw-w64-i686-libvorbis-1.3.7-2`
- `mingw-w64-i686-wavpack-5.9.0-1`
- `mingw-w64-i686-libxmp-4.7.0-1`
- `mingw-w64-i686-libogg-1.3.6-1`
- `mingw-w64-i686-opus-1.6.1-1`
- `mingw-w64-i686-gcc-libs-16.1.0-5`
- `mingw-w64-i686-winpthreads-14.0.0.r47.g0636d42e1-1`
- `mingw-w64-i686-libwinpthread-14.0.0.r47.g0636d42e1-1`

Dependency closure check:

- `objdump -p build-portable/bgdi.exe` and recursive DLL checks found no missing non-system DLLs after installing the decoder and MinGW runtime packages.

## Portable CMake Changes

The new portable CMake file:

- Reuses the runtime/interpreter source list from the Vita build.
- Omits the `VITASDK` requirement.
- Omits `vita.cmake`.
- Omits Vita packaging, `vita_create_self`, `vita_create_vpk`, and metadata.
- Does not apply the Vita-specific `sceClib*` remaps.
- Links desktop libraries directly: SDL2, SDL2_mixer, PNG, ZLIB, TRE, and math.
- Outputs `bgdi.exe` so BennuGD's existing standalone interpreter branch accepts a DCB filename argument.

## Configure Errors

`reports/portable_configure_1.log`

- Initial PowerShell/CMake argument split issue caused CMake to miss the intended compiler path.

`reports/portable_configure_2.log`

- Using the SDL2_mixer CMake config pulled in missing static decoder dependencies, starting with `libxmp`.
- Fix: use `find_path`/`find_library` for SDL2 and SDL2_mixer and link the DLL import libraries directly.

`reports/portable_configure_3.log`

- CMake 4 rejected the old TRE `cmake_minimum_required` compatibility range.
- Fix: configure with `-DCMAKE_POLICY_VERSION_MINIMUM=3.5`.

`reports/portable_configure_4.log`

- Quoting issue around `-DCMAKE_POLICY_VERSION_MINIMUM=3.5`.
- Fix: pass it as one quoted PowerShell argument.

`reports/portable_configure_5.log`

- Configure/generate completed.
- Remaining warning: TRE's old CMake minimum version is deprecated.

## Compiler Errors

`reports/portable_build_1.log`

- `fatal error: mod_draw_symbols.h: No such file or directory`
- `g_blit.c:2369:41: error: passing argument 4 of 'qsort' from incompatible pointer type`

Fixes:

- Added `modules/mod_draw` to portable include paths.
- Added GCC warning relaxation for legacy incompatible function pointer warnings.

`reports/portable_build_2.log`

- `mod_video.c:123:12: error: returning 'void *' from a function with return type 'int'`
- `mod_multi.c:213:51: error: implicit declaration of function 'string_get'`
- `mod_multi.c:216:5: error: implicit declaration of function 'string_discard'`

Fixes:

- Added `-Wno-error=implicit-function-declaration`.
- Added `-Wno-error=int-conversion`.
- Force-included `core/include/xstrings.h` for legacy string runtime declarations.

`reports/portable_build_3.log`

- `mod_regex.c:42:10: fatal error: tre/tre.h: No such file or directory`

Fix:

- Added `3rdparty/tre/lib` to portable include paths.

## Linker Errors

No final linker errors.

`reports/portable_build_4.log` reached the link command and emitted `sorr_portable.exe`, but PowerShell surfaced CMake's TRE deprecation warning as noisy stderr text.

`reports/portable_build_5.log` confirmed a no-op rebuild with `LASTEXITCODE=0`.

`reports/portable_build_6.log` relinked the executable as `bgdi.exe` with `LASTEXITCODE=0`.

## Launch Attempts

Attempt 1:

```powershell
Start-Process -FilePath build-portable\sorr_portable.exe -ArgumentList @('SorR.dat') -WorkingDirectory sorr-vita-master\data
```

Result:

- Reached `main()`.
- Failed to open the intended DCB because BennuGD treated `sorr_portable.exe` as a hand-made/game-stub executable.
- Output: `sorr_portable: doesn't exist or isn't version 7 DCB compatible`.

Fix:

- Set portable target output name to `bgdi.exe`, matching BennuGD's standalone interpreter check.

Attempt 2:

```powershell
Start-Process -FilePath build-portable\bgdi.exe -ArgumentList @('SorR.dat') -WorkingDirectory sorr-vita-master\data
```

Result after 8 seconds:

- Process stayed alive.
- Main window handle existed.
- Window title: `Streets of Rage Remake - v5.2`.
- Stderr:
  - `INFO: Called set_mode with 320x240x0`
  - `INFO: set_title(Streets of Rage Remake - v5.2)`
  - `INFO: Called set_mode with 320x240x0`
- Windows reported `Responding=False`.

Attempt 3:

Same launch command, 25-second probe.

Result:

- Process stayed alive.
- Window handle existed.
- Window title remained `Streets of Rage Remake - v5.2`.
- Windows still reported `Responding=False`.
- Process was stopped manually after the probe.

Attempt 4:

Same launch command, 10-second screenshot probe.

Result:

- Screenshot captured: `reports/portable_launch_window_4.png`.
- Screenshot shows a rendered `Streets of Rage Remake - v5.2` window with the night city scene.
- Process still reported `Responding=False` and was stopped manually.

## Current Blocker

The portable runtime now builds, reaches `main()`, opens the stripped `SorR.dat`, creates a real window, and renders a first scene. The next blocker is that the process does not become responsive after startup probes of 8, 10, and 25 seconds.

Recommended next debugging phase:

- Instrument the runtime/game loop after the second `set_mode` call.
- Check whether the BennuGD script is busy-waiting, blocked on input/audio, or stuck loading missing stage/menu data.
- Keep using the desktop `bgdi.exe` proof before starting iOS.
