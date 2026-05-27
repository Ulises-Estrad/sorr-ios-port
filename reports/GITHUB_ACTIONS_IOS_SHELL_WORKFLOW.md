# GitHub Actions iOS Shell Workflow

Date: 2026-05-27

## Purpose

`.github/workflows/ios-shell.yml` builds the milestone-1 iOS shell on GitHub Actions macOS.

The workflow proves:

- macOS runner is available,
- Xcode and the iOS Simulator SDK are available,
- CMake is available,
- SDL2 can be fetched and built for `iphonesimulator` `arm64`,
- the isolated `SorrIOSShell` CMake target can configure,
- the shell app can build as a simulator `.app`.

Current CI status:

```text
Build iOS shell for simulator arm64: succeeded in 4m 22s
```

The successful run completed:

- setup,
- checkout,
- game asset guard,
- tool version print,
- SDL2 fetch,
- SDL2 configure for iOS simulator,
- SDL2 build/install,
- iOS shell configure,
- iOS shell build,
- app artifact packaging,
- log/artifact upload.

It intentionally does not:

- upload game assets,
- bundle `SorR.dat`,
- load game data,
- start device signing,
- create an IPA,
- touch TestFlight/App Store paths,
- modify desktop or x64 known-good snapshots.

## Workflow Summary

Runner:

```yaml
runs-on: macos-15
```

Manual run:

```yaml
workflow_dispatch
```

Path-triggered runs:

- iOS shell CMake/source changes,
- tiny linked Bennu runtime source/header changes,
- workflow changes.

The workflow builds SDL2 from the official SDL2 source tarball:

```text
https://www.libsdl.org/release/SDL2-2.30.12.tar.gz
```

SDL2 is configured with:

```bash
-DCMAKE_SYSTEM_NAME=iOS
-DCMAKE_OSX_SYSROOT=iphonesimulator
-DCMAKE_OSX_ARCHITECTURES=arm64
-DSDL_STATIC=ON
-DSDL_SHARED=OFF
-DSDL_TEST=OFF
```

The shell is configured with:

```bash
cmake -S sorr-vita-master/cmake/ios \
  -B build-ios-shell-sim \
  -G Xcode \
  -DCMAKE_SYSTEM_NAME=iOS \
  -DCMAKE_OSX_SYSROOT=iphonesimulator \
  -DCMAKE_OSX_ARCHITECTURES=arm64 \
  -DCMAKE_PREFIX_PATH="$SDL2_IOS_PREFIX" \
  -DSORR_IOS_SDL2_ROOT="$SDL2_IOS_PREFIX" \
  -DSORR_IOS_BUNDLE_IDENTIFIER="dev.local.sorr.iosshell.ci"
```

The workflow installs SDL2 into:

```text
$GITHUB_WORKSPACE/_deps/sdl2-ios-sim
```

It exports that path for later steps:

```bash
echo "SDL2_IOS_PREFIX=$GITHUB_WORKSPACE/_deps/sdl2-ios-sim" >> "$GITHUB_ENV"
```

The shell configure step passes both `CMAKE_PREFIX_PATH` and `SORR_IOS_SDL2_ROOT` so it can use either SDL2's installed CMake package or the shell target's explicit root lookup.

Build destination:

```text
generic/platform=iOS Simulator
```

Code signing is disabled for simulator build steps:

```text
CODE_SIGNING_ALLOWED=NO
```

## Required Repo Exclusions

The repo should not contain game data or large local extraction outputs.

`.gitignore` now excludes:

- `SorR.dat`
- `data/`
- `palettes/`
- `savegame/`
- `*.fpg`
- `*.wav`
- `*.ogg`
- `*.smk`
- local build folders,
- local portable toolchains/dependencies,
- extracted/generated asset folders.

The workflow also has a guard step that fails if prohibited game assets are present in the checkout:

- `SorR.dat`
- `*.fpg`
- `*.wav`
- `*.ogg`
- `*.smk`

This is deliberate. The iOS shell build should be source-only.

## Expected Logs

Uploaded under:

```text
ci-artifacts/ios-shell/logs/
```

Expected log files:

- `asset-guard.log`
- `tool-versions.log`
- `sdl2-fetch.log`
- `sdl2-configure.log`
- `sdl2-build-install.log`
- `ios-shell-configure.log`
- `ios-shell-build.log`
- `package-artifacts.log`

The tool-version log should include:

- `sw_vers`
- `uname -a`
- `xcodebuild -version`
- `xcrun --sdk iphonesimulator --show-sdk-path`
- `cmake --version`

## Expected Artifacts

Uploaded artifact name:

```text
ios-shell-simulator-arm64
```

Expected contents:

- all logs above,
- `build-products/SorrIOSShell-iphonesimulator-arm64.zip` if the app bundle is produced.

The zip should contain:

```text
SorrIOSShell.app
```

No game data should appear in the uploaded artifact.

## Local Preflight Results

This Windows workspace cannot run the macOS job, but two local syntax/link preflights were run against the shell target after the workflow/CMake changes:

### Desktop syntax mode

Purpose:

- keep the original local shell syntax/link check working,
- compile with `SORR_IOS_SHELL_USE_SDL_MAIN_HANDLED=ON`,
- do not link `SDL2main`.

Command shape:

```powershell
cmake -S sorr-vita-master/cmake/ios `
  -B build-ios-shell-workflow-preflight `
  -G "MinGW Makefiles" `
  -DCMAKE_PREFIX_PATH="$PWD\portable-deps\msys2-mingw32\mingw32" `
  -DSORR_IOS_SHELL_USE_SDL_MAIN_HANDLED=ON
cmake --build build-ios-shell-workflow-preflight --verbose
```

Result:

```text
CONFIGURE_EXIT=0 BUILD_EXIT=0
```

Logs:

- `reports/ios_shell_workflow_preflight_configure.log`
- `reports/ios_shell_workflow_preflight_build.log`

### SDL main rewrite mode

Purpose:

- exercise the default SDL main rewrite path,
- link `SDL2::SDL2main` when the SDL2 package exposes it,
- more closely match the intended iOS workflow default.

Command shape:

```powershell
cmake -S sorr-vita-master/cmake/ios `
  -B build-ios-shell-workflow-preflight-sdlmain `
  -G "MinGW Makefiles" `
  -DCMAKE_PREFIX_PATH="$PWD\portable-deps\msys2-mingw32\mingw32"
cmake --build build-ios-shell-workflow-preflight-sdlmain --verbose
```

Result:

```text
CONFIGURE_EXIT=0 BUILD_EXIT=0
```

Logs:

- `reports/ios_shell_workflow_preflight_sdlmain_configure.log`
- `reports/ios_shell_workflow_preflight_sdlmain_build.log`

## Common Failure Modes

### Workflow cannot be dispatched from the local workspace

Observed locally:

- this workspace is not a Git checkout,
- `git` is not installed on PATH,
- `gh` is not installed on PATH,
- there is no GitHub remote or authenticated GitHub CLI session available.

Impact:

- the workflow file can be created and reviewed locally,
- but it cannot be pushed, dispatched, rerun, or inspected from this Windows workspace.

Fix:

- push the source-only repo to GitHub from an environment with `git`,
- run the workflow manually via `workflow_dispatch`, or
- install/authenticate GitHub CLI and dispatch:

```bash
gh workflow run ios-shell.yml
gh run list --workflow ios-shell.yml
gh run view <run-id> --log
```

### SDL iOS main target is missing

Likely first iOS CI failure if SDL's normal iOS main handling is used but the shell target does not link SDL2's main shim:

```text
Undefined symbols for architecture arm64: _main
```

Fix applied:

- `sorr-vita-master/cmake/ios/CMakeLists.txt` now links `SDL2::SDL2main` when the installed SDL2 CMake package exposes it.
- The link is skipped when `SORR_IOS_SHELL_USE_SDL_MAIN_HANDLED=ON`, because in that mode the shell keeps the normal `main` symbol and `SDL2main` expects `SDL_main`.
- The workflow configures the shell through `CMAKE_PREFIX_PATH="$RUNNER_TEMP/sdl2-ios-sim"` so SDL2's package targets can be discovered.

### Game assets present in checkout

Symptom:

```text
Game assets are present in the checkout
```

Cause:

- `SorR.dat`, FPG, WAV, OGG, or SMK files were committed or staged.

Fix:

- remove assets from git tracking,
- keep them local only,
- rerun the workflow.

### SDL2 tarball download fails

Symptom:

- `curl` fails in `sdl2-fetch.log`.

Fix:

- rerun first,
- if repeated, update the SDL2 version or URL in the workflow.

### SDL2 does not configure for iOS

Symptom:

- `sdl2-configure.log` fails around CMake/Xcode/iOS platform setup.

Fix:

- confirm the runner image has a valid iOS Simulator SDK,
- inspect `tool-versions.log`,
- consider switching `runs-on` between `macos-15` and `macos-latest`.

### SDL2 builds but install layout differs

Symptom:

- `ios-shell-configure.log` cannot find `SDL.h` or `libSDL2.a`.

Fix:

- inspect `sdl2-build-install.log` for the installed file list,
- adjust `CMAKE_PREFIX_PATH` or fall back to `SORR_IOS_SDL2_ROOT`,
- or switch to an SDL2 framework build path later.

### First CI failure: shell configure cannot find SDL2

Observed failure:

```text
SDL2 for iOS was not found.
Set SORR_IOS_SDL2_FRAMEWORK to SDL2.framework
or SORR_IOS_SDL2_ROOT to an SDL2 iOS install prefix.
```

Cause:

- The workflow built and installed SDL2, but the install prefix was only reconstructed locally in the shell configure step.
- The iOS shell CMake target has an explicit `SORR_IOS_SDL2_ROOT`/`SORR_IOS_SDL2_FRAMEWORK` path, and the workflow was only passing `CMAKE_PREFIX_PATH`.

Fix applied:

- SDL2 now installs into a stable workspace path: `$GITHUB_WORKSPACE/_deps/sdl2-ios-sim`.
- The SDL2 configure/install step writes `SDL2_IOS_PREFIX` to `$GITHUB_ENV`.
- The iOS shell configure step passes `-DSORR_IOS_SDL2_ROOT="$SDL2_IOS_PREFIX"`.
- The configure step now logs a discovery probe before CMake:

```bash
find "$GITHUB_WORKSPACE/_deps" -maxdepth 5 \
  \( -name "SDL2.framework" -o -name "libSDL2*.a" -o -name "SDL.h" \) \
  -print | sort
```

Expected next run result:

- `ios-shell-configure.log` should show `SDL2_IOS_PREFIX` and at least one installed SDL2 header/library path.
- The shell configure should advance past SDL2 discovery.
- The next possible blocker is expected to be a simulator link/build issue, not SDL2 prefix discovery.

### Second CI failure: SDL2 installed but `find_path` misses `SDL.h`

Observed log:

```text
SDL2_IOS_PREFIX=/Users/runner/work/sorr-ios-port/sorr-ios-port/_deps/sdl2-ios-sim
SDL2 discovery debug:
.../_deps/sdl2-ios-sim/include/SDL2/SDL.h
.../_deps/sdl2-ios-sim/lib/libSDL2.a
.../_deps/sdl2-ios-sim/lib/libSDL2main.a
CMake Error at CMakeLists.txt:67 (find_path):
  Could not find SDL2_INCLUDE_DIR using the following files: SDL.h
```

Cause:

- SDL2 was installed correctly, and the debug probe proved the header and libraries existed.
- During iOS cross-configuration, CMake's normal `find_path`/`find_library` behavior can apply SDK/root-path search rules that do not accept the workspace install prefix as intended.

Fix applied:

- The `SORR_IOS_SDL2_ROOT` branch now uses `PATHS` with `NO_DEFAULT_PATH` and `NO_CMAKE_FIND_ROOT_PATH` for SDL2 header/library lookup.
- The same explicit-root branch now discovers and links `libSDL2main.a` when `SORR_IOS_SHELL_USE_SDL_MAIN_HANDLED` is off.
- The configure log now prints the selected SDL2 include directory, SDL2 library, and SDL2main library.

Expected next run result:

- `ios-shell-configure.log` should print `Using SDL2 include directory`, `Using SDL2 library`, and `Using SDL2main library`.
- Configure should advance past SDL2 include/library resolution.
- The next blocker, if any, should be in Xcode generation or simulator linking.

### Third CI failure: static SDL2 missing iOS frameworks at link

Observed progress:

- Configure passed.
- Xcode project generation passed.
- C sources compiled for `arm64-apple-ios18.5-simulator`.
- The link command included `libSDL2main.a` and `libSDL2.a`.

Observed failure:

```text
Undefined symbols for architecture arm64:
  "_CBAdvertisementDataLocalNameKey"
  "_OBJC_CLASS_$_CBCentralManager"
  "_CHHapticDynamicParameterIDHapticIntensityControl"
  "_OBJC_CLASS_$_CMMotionManager"
  "_MTLCreateSystemDefaultDevice"
  "_OBJC_CLASS_$_EAGLContext"
  "_glActiveTexture"
  "_glBindFramebuffer"
  "_kEAGLColorFormatRGBA8"
```

Cause:

- Static SDL2 on iOS does not bring all Apple framework dependencies transitively through the archive.
- SDL2's linked objects reference BLE HID, haptics, motion/controller support, Metal rendering, and OpenGL ES rendering.

Fix applied:

- Added the missing framework links to the iOS shell target:
  - `CoreBluetooth`
  - `CoreHaptics`
  - `CoreMotion`
  - `Metal`
  - `OpenGLES`
- Existing framework links for `AudioToolbox`, `AVFoundation`, `CoreGraphics`, `Foundation`, `GameController`, `QuartzCore`, and `UIKit` remain.

Expected next run result:

- Link should advance past the missing SDL2 framework symbols.
- The next likely blocker, if any, should be app bundle packaging/signing metadata or artifact collection.

### iOS shell configure fails on ZLIB

Symptom:

```text
Could NOT find ZLIB
```

Fix:

- verify the iOS SDK path in `tool-versions.log`,
- if needed, update the CMake shell target to use SDK zlib explicitly.

### iOS shell build fails before app bundle

Symptom:

- no `SorrIOSShell.app` found in `package-artifacts.log`.

Fix:

- inspect `ios-shell-build.log`,
- if failure is SDL main/app entry related, test `SORR_IOS_SHELL_USE_SDL_MAIN_HANDLED=ON` in a follow-up branch,
- if failure is signing related, keep simulator destination and `CODE_SIGNING_ALLOWED=NO`.

## Next Step After A Successful Shell Build

After the workflow produces `SorrIOSShell.app` for simulator:

1. Add a simulator run job or a manual Mac/cloud-Mac launch step.
2. Capture runtime logs proving:
   - app entry reached,
   - `SDL_Init` begin/end,
   - SDL video/events/timer init,
   - SDL window/renderer created,
   - `bgdrtm_entry` reached,
   - idle loop running.
3. Generalize the x64-safe runtime fixes from `_WIN64` to a portable 64-bit guard.
4. Build the shell with the x64-safe runtime path still not loading `SorR.dat`.
5. Only then start the iOS data layout shim.

Do not bundle or load game data until the shell build and launch are proven.
