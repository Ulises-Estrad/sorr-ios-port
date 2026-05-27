# iOS App Shell Notes

Date: 2026-05-27

## Scope

Milestone 1 only. The new target is an iOS SDL shell that initializes SDL, initializes video/events/timer, reaches a minimal Bennu runtime entry, and idles in an event loop.

Not done in this milestone:

- No `SorR.dat` load.
- No game data bundle.
- No savegame/config access.
- No audio assets.
- No broad pointer remediation.
- No touch input bridge.

The desktop known-good target and data state were left untouched.

## Files Created

- `sorr-vita-master/cmake/ios/CMakeLists.txt`
  - Defines the isolated `SorrIOSShell` CMake target.
  - Accepts `SORR_IOS_SDL2_FRAMEWORK` or `SORR_IOS_SDL2_ROOT`.
  - Links only the tiny Bennu runtime entry subset needed for the shell probe.
- `sorr-vita-master/cmake/ios/Info.plist.in`
  - Minimal iOS app bundle metadata.
  - Sets fullscreen landscape orientation for the shell.
- `sorr-vita-master/cmake/ios/src/sorr_ios_shell_main.c`
  - SDL app entry.
  - Initializes SDL/video/events/timer.
  - Creates a simple window and renderer.
  - Calls `bgdrtm_entry(argc, argv)` without loading game data.
  - Runs a responsive idle loop.

## What The Shell Does

- Launches as an app bundle target when configured with an iOS/Xcode toolchain.
- Initializes SDL with `SDL_Init(0)`.
- Initializes the SDL video, events, and timer subsystems.
- Creates a window titled `SorR iOS Shell`.
- Creates an accelerated/vsync SDL renderer.
- Allocates a tiny `globaldata` buffer for the globals touched by `bgdrtm_entry`.
- Calls the real Bennu runtime entry function: `bgdrtm_entry(argc, argv)`.
- Logs that `SorR.dat` is intentionally not loaded.
- Enters a small `SDL_PollEvent` idle/render loop.
- Attempts clean SDL shutdown if an `SDL_QUIT` event is received.

## What The Shell Intentionally Does Not Do

- Does not load `SorR.dat`.
- Does not read from `sorr-vita-master/data`.
- Does not bundle any game data.
- Does not open savegame/config files.
- Does not initialize SDL_mixer or audio.
- Does not load WAV/OGG/SMK assets.
- Does not create Bennu game processes.
- Does not run the interpreter.
- Does not implement touch controls.
- Does not remediate the pointer-through-int ABI.
- Does not modify the known-good desktop portable target.

## Build System Choice

Build system: CMake/Xcode-oriented iOS app target.

Reason:

- Keeps the original Vita CMake untouched.
- Keeps the known-good desktop portable CMake untouched.
- Lets the iOS shell live in its own build area.
- Can reuse the portable source list later, but does not pull the pointer-heavy runtime for this milestone.

The target name is:

```text
SorrIOSShell
```

It is declared as a `MACOSX_BUNDLE` executable so CMake can generate an iOS app bundle when configured with an iOS/Xcode toolchain.

## Intended iOS Configure Command

Example framework-based setup on macOS:

```bash
cmake -S sorr-vita-master/cmake/ios \
  -B build-ios-shell \
  -G Xcode \
  -DCMAKE_SYSTEM_NAME=iOS \
  -DSORR_IOS_SDL2_FRAMEWORK=/absolute/path/to/SDL2.framework
```

Example install-prefix setup:

```bash
cmake -S sorr-vita-master/cmake/ios \
  -B build-ios-shell \
  -G Xcode \
  -DCMAKE_SYSTEM_NAME=iOS \
  -DSORR_IOS_SDL2_ROOT=/absolute/path/to/sdl2-ios-prefix
```

Build:

```bash
cmake --build build-ios-shell --config Debug
```

## SDL2 iOS Setup

The CMake target accepts either:

- `SORR_IOS_SDL2_FRAMEWORK=/path/to/SDL2.framework`
- `SORR_IOS_SDL2_ROOT=/path/to/prefix`
- or an SDL2 CMake package exposing `SDL2::SDL2` / `SDL2::SDL2-static`

If `SDL2.framework` is used, the target adds the framework include path, links `-framework SDL2`, and marks the framework for Xcode embedding/signing.

Apple platform frameworks linked for the iOS target:

- `AudioToolbox`
- `AVFoundation`
- `CoreGraphics`
- `Foundation`
- `GameController`
- `QuartzCore`
- `UIKit`

`ZLIB` is found and linked because the tiny `bgdrtm_entry` source currently includes runtime headers that transitively include zlib-backed file types. No compressed game data is loaded in this milestone.

## SDL_MAIN_HANDLED Status

Default for the iOS shell target:

```text
SORR_IOS_SHELL_USE_SDL_MAIN_HANDLED=OFF
```

Reason:

- SDL iOS builds usually expect SDL's app entry handling and `main` rewrite path.
- The target should first be tested on macOS/Xcode with the default OFF setting.

Local Windows syntax check used:

```text
SORR_IOS_SHELL_USE_SDL_MAIN_HANDLED=ON
```

Reason:

- The syntax check was intentionally compiled as a Windows executable using the local desktop SDL dependency, not as an iOS app.
- That check only verifies source/CMake shape; it does not prove iOS main handling.

Current conclusion:

- `SDL_MAIN_HANDLED` should remain OFF for the first real iOS/Xcode attempt.
- If the iOS app fails before reaching `main`, retest with `SORR_IOS_SHELL_USE_SDL_MAIN_HANDLED=ON` and an explicit SDL-compatible app entry path.

## iOS Simulator/Device Target

CMake properties added:

- supported platforms: `iphoneos iphonesimulator`
- target device family: `1,2` (iPhone and iPad)
- orientations: landscape left/right
- fullscreen required

No actual iOS simulator/device build was run in this Windows environment.

## Runtime Entry Strategy

The shell links only a tiny Bennu runtime subset:

- `core/bgdrtm/src/fmath.c`
- `core/bgdrtm/src/misc.c`
- `core/bgdrtm/src/strings.c`

The shell creates a minimal `globaldata` buffer large enough for the runtime globals touched by `bgdrtm_entry`, then calls:

```c
bgdrtm_entry(argc, argv);
```

This reaches the real Bennu runtime entry without:

- loading a DCB,
- creating a Bennu process,
- invoking the interpreter,
- opening `SorR.dat`,
- opening any assets,
- initializing audio,
- entering pointer-heavy script execution paths.

Expected log sequence:

```text
SORR iOS shell: app entry
SORR iOS shell: SDL_Init ok
SORR iOS shell: SDL video/events/timer init ok
SORR iOS shell: reached Bennu runtime handoff probe ...
SORR iOS shell: bgdrtm_entry returned
SORR iOS shell: SorR.dat intentionally not loaded in milestone 1
SORR iOS shell: entering responsive idle loop
```

## Shell Runtime Log Checklist

- [ ] app entry reached: `SORR iOS shell: app entry`
- [ ] `SDL_Init` begin/end observed: `SDL_Init ok`
- [ ] video/events/timer init observed: `SDL video/events/timer init ok`
- [ ] `SDL_CreateWindow` success: no `SDL_CreateWindow failed` log
- [ ] `SDL_CreateRenderer` success: no `SDL_CreateRenderer failed` log
- [ ] `bgdrtm_entry` call reached: `reached Bennu runtime handoff probe`
- [ ] `bgdrtm_entry` returned: `bgdrtm_entry returned`
- [ ] idle event loop running: `entering responsive idle loop`
- [ ] clean quit, if lifecycle delivers quit: `clean shutdown`

## Lifecycle Notes

The shell does:

1. `SDL_Init(0)`
2. `SDL_InitSubSystem(SDL_INIT_VIDEO | SDL_INIT_EVENTS | SDL_INIT_TIMER)`
3. `SDL_CreateWindow("SorR iOS Shell", ...)`
4. `SDL_CreateRenderer(...)`
5. minimal `bgdrtm_entry(...)`
6. responsive idle loop using `SDL_PollEvent`
7. simple clear/present frame
8. clean shutdown on `SDL_QUIT` if delivered

On iOS, clean shutdown may be app-lifecycle dependent rather than a normal desktop `SDL_QUIT`.

## Local Configure/Build Check

This machine is Windows, so it cannot perform a real iOS/Xcode build. A desktop syntax/link check was run using the bundled local dependencies to catch source-level errors.

Command used:

```powershell
$deps = (Resolve-Path ".\portable-deps\msys2-mingw32\mingw32").Path
$env:PATH = (Resolve-Path ".\portable-tools\w64devkit\bin").Path + ";" + (Join-Path $deps "bin") + ";" + $env:PATH
.\portable-tools\w64devkit\bin\cmake.exe -S "sorr-vita-master/cmake/ios" -B "build-ios-shell-desktop-syntax" -G "MinGW Makefiles" -DCMAKE_PREFIX_PATH="$deps" -DSORR_IOS_SDL2_ROOT="$deps" -DSORR_IOS_SHELL_USE_SDL_MAIN_HANDLED=ON
.\portable-tools\w64devkit\bin\cmake.exe --build "build-ios-shell-desktop-syntax" --verbose
```

Result:

```text
CONFIG=0 BUILD=0
```

This confirms the new shell source, CMake target shape, tiny Bennu runtime entry subset, SDL include/link path, ZLIB include/link path, and dead-code stripping strategy are valid enough for a local desktop syntax/link pass. It does not prove an iOS app launch.

Logs:

- `reports/ios_shell_desktop_syntax_configure.log`
- `reports/ios_shell_desktop_syntax_build.log`

## Compile Errors Encountered

Initial desktop syntax attempt failed when the shell included `bgdrtm.h` directly:

```text
fatal error: zlib.h: No such file or directory
```

Fix:

- Avoid direct `bgdrtm.h` include in the shell source.
- Forward-declare `bgdrtm_entry`.
- Add explicit `find_package(ZLIB REQUIRED)` to the iOS shell CMake because `misc.c` still includes runtime headers that need zlib headers.

Current compile status:

- Local desktop syntax compile succeeds.
- Real iOS compile not yet run.

## Linker Errors Encountered

After adding the tiny real `bgdrtm_entry` subset, the local desktop syntax link initially failed on unused functions inside `strings.c`:

```text
undefined reference to `file_seek'
undefined reference to `file_readUint32A'
undefined reference to `file_read'
undefined reference to `c_upper'
undefined reference to `c_lower'
```

Fix:

- Add `-ffunction-sections -fdata-sections`.
- Add dead-code stripping:
  - Apple: `-Wl,-dead_strip`
  - GNU syntax check: `-Wl,--gc-sections`

Current link status:

- Local desktop syntax link succeeds.
- Real iOS link not yet run.

## Whether The App Reaches Runtime Entry

Source-level status:

- Yes, the shell calls the real `bgdrtm_entry(argc, argv)`.

Local build status:

- The shell target compiles and links in the desktop syntax check.

iOS runtime status:

- Not yet verified on simulator/device because this environment is Windows and has no Xcode/iOS SDK.

## What Remains Unproven Without macOS/Xcode

- Whether CMake generates the Xcode iOS app bundle cleanly.
- Whether SDL2.framework or an SDL2 iOS static install links cleanly on simulator/device.
- Whether the default `SDL_MAIN_HANDLED=OFF` path reaches `main` under SDL iOS.
- Whether iOS app lifecycle events deliver a normal `SDL_QUIT` or require lifecycle-specific handling.
- Whether the window/renderer creation path works on simulator and device.
- Whether the log stream appears in Xcode/device console as expected.

## Future Blockers Kept Open

These are intentionally documented but not solved in this milestone:

- pointer-through-int ABI
- iOS data layout
- savegame writable path
- SDL_mixer setup
- OGG/Vorbis decoder setup
- touch input bridge
- full `SorR.dat` load/render

## Current Blocker

The current blocker for proving the app on actual iOS is environment/dependency setup:

- Need macOS/Xcode/iOS SDK.
- Need SDL2 built for iOS as a framework or static install prefix.

Once those are available, the next check is whether the default SDL iOS main handling reaches the log line:

```text
SORR iOS shell: app entry
```
