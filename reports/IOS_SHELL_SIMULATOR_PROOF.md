# iOS Shell Simulator Proof

Date: 2026-05-27

## Scope

Goal for this phase: build and run the iOS shell only.

Guardrails honored:

- Did not load `SorR.dat`.
- Did not bundle game data.
- Did not modify known-good desktop files.
- Did not start pointer ABI remediation.

## Result

Status: blocked before iOS configure/build.

The active tool session is still a Windows/PowerShell environment, not macOS/Xcode. Xcode, `xcodebuild`, `xcrun`, CMake on PATH, and the iOS simulator SDK are not available from this session. No iOS app was configured, built, installed, or launched.

Evidence log:

- `reports/ios_shell_tool_verify.log`
- `reports/ios_shell_xcode_configure_attempt.log`

## Tool Verification

Command result summary from `reports/ios_shell_tool_verify.log`:

```text
OS=Microsoft Windows NT 10.0.26200.0
MACHINE=ULISES-LAPTOP
xcodebuild=NOT_FOUND
xcrun=NOT_FOUND
cmake=NOT_FOUND
git=NOT_FOUND
ninja=NOT_FOUND
sw_vers=NOT_FOUND
uname=NOT_FOUND
SDK_CHECK=not_run_xcrun_missing
```

The only CMake available was the bundled Windows tool:

```text
portable-tools/w64devkit/bin/cmake.exe
```

That is sufficient for the previous desktop syntax/link check, but not for a real iOS/Xcode app proof.

## iOS Simulator SDK

Not verified.

Reason:

```text
xcrun=NOT_FOUND
```

The required command could not be run:

```bash
xcrun --sdk iphonesimulator --show-sdk-path
```

## SDL2 For iOS

No SDL2 iOS framework or iOS install prefix was found in the workspace.

Workspace search result:

```text
SDL2_FRAMEWORKS_FOUND=0
```

Only the existing desktop/MinGW SDL2 dependency was present:

```text
portable-deps/msys2-mingw32/mingw32/include/SDL2/SDL.h
portable-deps/msys2-mingw32/mingw32/lib/libSDL2.a
portable-deps/msys2-mingw32/mingw32/lib/libSDL2.dll.a
portable-deps/msys2-mingw32/mingw32/lib/libSDL2main.a
```

This is not usable as the iOS SDL2 dependency.

## Configure Attempt

Attempted command using the bundled Windows CMake:

```powershell
.\portable-tools\w64devkit\bin\cmake.exe `
  -S "sorr-vita-master/cmake/ios" `
  -B "build-ios-shell-sim-proof" `
  -G "Xcode" `
  -DCMAKE_SYSTEM_NAME=iOS
```

Result:

```text
CMake Error: Could not create named generator Xcode
```

Reason:

- The current CMake executable is the Windows bundled CMake.
- Xcode generator is not available outside macOS/Xcode.

The temporary failed configure directory was removed after the attempt. Logs were kept in `reports/`.

## Build For iOS Simulator

Not run.

Blocked by:

- no macOS/Xcode session exposed to tools,
- no `xcodebuild`,
- no `xcrun`,
- no iOS simulator SDK,
- no SDL2 iOS dependency.

## Simulator Launch

Not run.

Blocked by the same environment/toolchain issues above.

## Runtime Log Checklist

- [ ] app entry reached: not proven
- [ ] `SDL_Init` begin/end: not proven
- [ ] SDL video init: not proven
- [ ] `SDL_CreateWindow` success: not proven
- [ ] `SDL_CreateRenderer` success: not proven
- [ ] `bgdrtm_entry` reached: not proven
- [ ] idle loop running: not proven
- [ ] clean quit: not proven

## Expected First Success Log

When run from a real macOS/Xcode+iOS simulator environment, the target should still be validated against:

```text
SORR iOS shell: app entry
SORR iOS shell: SDL_Init ok
SORR iOS shell: SDL video/events/timer init ok
SORR iOS shell: reached Bennu runtime handoff probe argc=...
SORR iOS shell: bgdrtm_entry returned
SORR iOS shell: SorR.dat intentionally not loaded in milestone 1
SORR iOS shell: entering responsive idle loop
```

## Next Required Action

Run the steps in `reports/MACOS_XCODE_IOS_BUILD_STEPS.md` from a tool session that actually exposes macOS/Xcode, or manually from Terminal on the Mac:

```bash
xcodebuild -version
xcrun --sdk iphonesimulator --show-sdk-path
cmake --version
```

Then provide or build SDL2 for iOS and configure with either:

```bash
-DSORR_IOS_SDL2_FRAMEWORK=/absolute/path/to/SDL2.framework
```

or:

```bash
-DSORR_IOS_SDL2_ROOT=/absolute/path/to/sdl2-ios-prefix
```

Stop once the shell reaches:

```text
SORR iOS shell: entering responsive idle loop
```

