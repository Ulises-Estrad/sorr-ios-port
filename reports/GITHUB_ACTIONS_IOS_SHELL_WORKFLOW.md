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
- the shell app can build as a simulator `.app`,
- the built app can be installed and launched in an available iOS Simulator,
- shell startup logs can be captured from the simulator.

Current CI status:

```text
Simplify iOS simulator launch diagnostics #13
Commit: 66dcbc9
Status: Success
Total duration: 10m 39s
Job: Build iOS shell for simulator arm64, 10m 35s
Artifact: ios-shell-simulator-arm64, 2.43 MB

Add iOS data layout scaffold proof
Commit: 2d3e5a6
Run: https://github.com/Ulises-Estrad/sorr-ios-port/actions/runs/26537165603
Status: Success
Artifact: ios-shell-simulator-arm64, 2.34 MB
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
- simulator boot/install/launch,
- required shell startup marker validation,
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

The successful run proves the shell:

- launches in an iOS Simulator,
- reaches app entry,
- initializes SDL,
- initializes SDL video/events/timer,
- resolves the app bundle resource root,
- resolves `Library/Application Support/SORR`,
- creates writable `savegame`, `xbox`, and `logs` directories,
- writes and reads a tiny proof file under `logs`,
- creates an SDL window and renderer,
- reaches and returns from the Bennu runtime handoff probe,
- explicitly does not load `SorR.dat`,
- enters the responsive idle loop.

D2 adds a device-only data import/storage probe while keeping the simulator job shell/data-layout only. The probe uses iOS file sharing rather than bundling private data.

The workflow now also has a device artifact job:

```text
Build iOS shell unsigned IPA for device arm64
```

That job builds an `iphoneos` `arm64` shell app, packages `Payload/SorrIOSShell.app`, uploads a shell-only IPA, and verifies the IPA contains no game data.

Latest D1 result:

```text
Commit: 236455a
Run: https://github.com/Ulises-Estrad/sorr-ios-port/actions/runs/26540409093
Device job: Build iOS shell unsigned IPA for device arm64
Device job result: success
Artifact: ios-shell-device-unsigned-arm64
Artifact size: 456283 bytes
IPA inside artifact: build-products/SorrIOSShell-device-adhoc.ipa
Physical install route: Windows + Sideloadly
Physical iPhone result: app launches to shell-only dark idle screen
Game data/assets: not bundled
```

Current D2 physical proof:

```text
Passing Actions run: 26543157389
Artifact: ios-shell-d2-sorr-import-device-arm64
IPA inside artifact: build-products/SorrIOSShell-d2-sorr-import-adhoc.ipa
Import route: iOS Files app / file sharing via Documents/SORR_IMPORT
Local-only import package: out/local-only/SORR_IMPORT.zip
ZIP SHA256: B2E2F3901C65BE0FA3D739D3716C74C37AE13FB9BDF7535ADC487D9A3D22C72A
Physical result: direct layout detected, staging copied, SorR.dat opened, mod/system.txt found, savegame/xbox/logs writable, probe log OK
Runtime/game execution: intentionally skipped
Game rendering: intentionally skipped
Game data/assets in IPA: not bundled
```

Previous D2 result:

```text
Commit: 5d9036f
Run: https://github.com/Ulises-Estrad/sorr-ios-port/actions/runs/26541978819
Device job: Build iOS shell unsigned IPA for device arm64
Device job result: success
Artifact: ios-shell-d2-data-import-device-arm64
Artifact size: 459831 bytes
IPA inside artifact: build-products/SorrIOSShell-d2-data-import-adhoc.ipa
Physical import/storage result: superseded by completed SORR_IMPORT proof
```

Updated D2 `SORR_IMPORT` physical proof is complete.

Current D3A audio/stability diagnostic target:

```text
Artifact: ios-shell-d3a-audio-diagnostics-device-arm64
IPA inside artifact: build-products/SorrIOSShell-d3a-audio-diagnostics-adhoc.ipa
Purpose: physical-device first-render stability diagnostics with minimal real SDL audio, named audio no-op counters, and music/BGM file-open diagnostics
Diagnostics path on phone: On My iPhone/SorrIOSShell/SORR_DIAGNOSTICS/ios_d3_runtime_stability_probe.txt
Dense logging window: runtime_ms=240000 through runtime_ms=330000 at one-second cadence
Game data/assets in IPA: not bundled
```

This target does not start D4a touch input. It keeps the D2 data path and D3 render path intact while replacing the pure audio stub with a minimal SDL2 audio backend. WAV effects are loaded through Bennu's virtual file layer and queued to SDL audio. That SFX-only path proved useful diagnostically, but it is now superseded by the D3A music target because inert/silent BGM is not an acceptable endpoint.

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

The SDL2 build keeps code signing disabled because it only builds static SDL2 libraries:

```text
CODE_SIGNING_ALLOWED=NO
```

The shell app build lets Xcode create a simulator-local ad-hoc signature:

```bash
CODE_SIGNING_ALLOWED=YES
CODE_SIGNING_REQUIRED=NO
CODE_SIGN_IDENTITY="-"
```

Before simulator install, the workflow verifies and displays that signature:

```bash
codesign --verify --deep --strict --verbose=2 "$APP_PATH"
codesign --display --verbose=4 "$APP_PATH"
```

This does not use provisioning profiles, device signing, IPA export, TestFlight, or App Store signing. It only gives the CI-built simulator `.app` the local signature SpringBoard sees during `simctl launch`.

The launch step also prints key bundle metadata before install:

- full `Info.plist` via `plutil -p`,
- bundle identifier,
- `UILaunchScreen`,
- supported orientations,
- bundle file list,
- installed app container path when available.
- installed app metadata from `simctl listapps`,
- host-side CoreSimulator/LaunchServices diagnostics when launch fails.

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
- `ios-shell-simulator-launch.log`
- `simulator/simctl-launch.log`
- `simulator/sorr-ios-shell-stdout.log`
- `simulator/sorr-ios-shell-stderr.log`
- `simulator/sorr-ios-shell-log-stream.log`
- `simulator/sorr-ios-shell-log-show.log`
- `simulator/sorr-ios-shell-system-log-show.log`
- `simulator/sorr-ios-shell-host-coresimulator-log-show.log`
- `simulator/sorr-ios-shell-combined.log`
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

## Simulator Launch Proof

After the simulator `.app` build succeeds, the workflow now:

1. Selects the first available iPhone simulator from `xcrun simctl list devices available --json`.
2. Shuts down and erases that simulator to clear stale install placeholders.
3. Boots it and waits for `simctl bootstatus`.
4. Prints `Info.plist` diagnostics.
5. Verifies the Xcode-produced simulator app signature.
6. Uninstalls any stale copy of the same bundle id.
7. Installs `SorrIOSShell.app`.
8. Launches the bundle with retries.
9. Captures simulator logs for 20 seconds.
10. Terminates the shell.
11. Combines `simctl launch`, app stdout/stderr, `log stream`, and `log show` output.
12. Fails the job if `simctl launch` fails or if required startup markers are missing.

Required markers:

```text
SORR iOS shell: app entry
SORR iOS shell: SDL_Init ok
SORR iOS shell: SDL video/events/timer init ok
SORR iOS shell: bundle root path=
SORR iOS shell: support root path=
SORR iOS shell: D2 import inbox path=
SORR iOS shell: D2 canonical data path=
SORR iOS shell: savegame dir created
SORR iOS shell: xbox dir created
SORR iOS shell: logs dir created
SORR iOS shell: data layout test file write/read ok
SORR iOS shell: D2 import layout detected=none
SORR iOS shell: D2 waiting for data import path=
SORR iOS shell: D2 staging result=not started
SORR iOS shell: D2 probe log write/read ok
SORR iOS shell: SDL_CreateWindow success
SORR iOS shell: SDL_CreateRenderer success
SORR iOS shell: reached D2 runtime skip probe
SORR iOS shell: Bennu runtime intentionally skipped in D2
SORR iOS shell: SorR.dat intentionally not loaded or executed in D2
SORR iOS shell: entering responsive idle loop
```

The shell source emits separate success lines for `SDL_CreateWindow` and `SDL_CreateRenderer`. It also logs the bundle resource root, writable Application Support scaffold, D2 `Documents/SORR_IMPORT` inbox, canonical `Library/Application Support/SORR` root, and D2 runtime skip. `entering responsive idle loop` remains the final shell-only liveness marker.

Latest launch failure:

```text
FBSOpenApplicationServiceErrorDomain code=1
The request was denied by service delegate (SBMainWorkspace).
```

The app installed and appeared in `simctl listapps`, but no shell logs appeared. Host-side diagnostics showed install coordination briefly describing the bundle as invalid and applying a prevent-launch limitation before process entry.

Current fix:

- let Xcode produce the simulator-local ad-hoc app signature,
- verify that signature before install,
- stop post-build manual deep signing of the app bundle.

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

### First simulator launch failure: SpringBoard denied app launch

Observed failure:

```text
An error was encountered processing the command (domain=FBSOpenApplicationServiceErrorDomain, code=1):
Simulator device failed to launch dev.local.sorr.iosshell.ci.
The request was denied by service delegate (SBMainWorkspace).
```

Observed progress:

- The workflow selected an available iPhone simulator.
- `simctl bootstatus` completed.
- The app path and bundle id were found.
- The failure occurred at `simctl launch`, before shell runtime log markers appeared.

Likely cause:

- The app was built with `CODE_SIGNING_ALLOWED=NO`.
- `simctl install` can accept the bundle, but SpringBoard may still deny launch of an unsigned simulator app.

Fix applied:

- Keep Xcode build signing disabled to avoid device/provisioning paths.
- Add a local simulator-only ad-hoc `codesign -` pass before `simctl install`.
- Verify and display the ad-hoc signature before boot/install/launch continues.

Follow-up result:

- Ad-hoc signing succeeded and verified.
- The app still failed at `simctl launch` with the same `SBMainWorkspace` denial.

Second fix applied:

- Add a minimal `LaunchScreen.storyboard` resource to the app bundle.
- Set `UILaunchStoryboardName` to `LaunchScreen` instead of leaving the generated Info.plist launch storyboard key empty.
- Print bundle metadata before install to verify the launch storyboard and bundle id in CI.
- Capture `simctl launch` failures without aborting immediately, so `log show` and combined simulator logs are still uploaded.

Second follow-up result:

- `Info.plist` showed `UILaunchStoryboardName=LaunchScreen`.
- The ad-hoc signature remained valid.
- The app installed and `simctl get_app_container` returned the installed `.app` path.
- `simctl launch` was still denied by `SBMainWorkspace`.
- The narrow app log filter did not expose the underlying SpringBoard/FrontBoard reason.

Third fix applied:

- Make the shell bundle iPhone-only for this simulator proof (`UIDeviceFamily=1`) instead of universal iPhone/iPad.
- Add `CFBundleDisplayName`.
- Add portrait to the shell's supported orientations so launch is not blocked by the simulator's initial portrait orientation.
- Add `UIApplicationSupportsIndirectInputEvents` and hide the status bar for a more conventional SDL iOS shell plist.
- Capture broader SpringBoard, FrontBoard, RunningBoard, LaunchServices, install, and bundle-id logs into `simulator/sorr-ios-shell-system-log-show.log`.
- Touch stdout/stderr log files before launch so artifact collation is quiet even when the process never starts.

Expected next run result:

- `simctl launch` should advance past the SpringBoard denial.
- If launch is still denied, the uploaded system log should include the deeper FrontBoard/SpringBoard reason.

### Fourth simulator launch failure: app still denied before process start

Observed progress:

- `LaunchScreen.storyboardc` was present in the `.app`.
- `Info.plist` had `UILaunchStoryboardName=LaunchScreen`, `UIDeviceFamily=1`, `CFBundleDisplayName`, and portrait plus landscape orientations.
- The app installed and `simctl listapps` returned the bundle metadata.
- `simctl launch` still failed before app entry with `FBSOpenApplicationServiceErrorDomain code=1` and `SBMainWorkspace`.

Cause under test:

- The failure is still pre-entry, so the shell code is not running yet.
- The compiled storyboard launch screen is not missing, but it is still one more launch-time dependency in a minimal CI shell.
- The failed launch may also be a transient SpringBoard/LaunchServices state immediately after install.

Fix applied:

- Replace `UILaunchStoryboardName=LaunchScreen` with an empty `UILaunchScreen` dictionary.
- Remove the storyboard resource from the CMake app target.
- Print the full app bundle tree with `find "$APP_PATH" -maxdepth 3 -print`.
- Retry `simctl launch` up to four times with short backoff.
- Capture host-side CoreSimulator logs to `simulator/sorr-ios-shell-host-coresimulator-log-show.log`.
- Add explicit shell markers for `SDL_CreateWindow success` and `SDL_CreateRenderer success`.

Expected next run result:

- If the denial was launch-screen or transient app-registration related, one of the retry attempts should start the shell and emit the required markers.
- If launch is still denied before process start, the host CoreSimulator log should provide the lower-level launch reason.

### Fifth simulator launch failure: placeholder cleared but process launch still denied

Observed progress:

- The selected simulator was erased before boot.
- Any stale bundle registration was uninstalled before install.
- `CFBundleDisplayName` matched `SorrIOSShell`.
- The app installed and `simctl listapps` reported `isPlaceholder=0`-style final metadata for the real app.
- LaunchServices registration succeeded for `dev.local.sorr.iosshell.ci`.

Observed failure:

```text
FBSOpenApplicationServiceErrorDomain code=1
The request was denied by service delegate (SBMainWorkspace).
FBProcessExit Code=64 "The process failed to launch."
RBSRequestErrorDomain Code=5 "Launch failed."
```

Cause under test:

- The failure is still pre-entry, so the shell code is not running yet.
- The placeholder/app-registration issue appears cleared.
- The remaining launch denial may be caused by building the `.app` with `CODE_SIGNING_ALLOWED=NO` and applying a manual deep ad-hoc signature afterward.

Fix applied:

- Let Xcode produce the simulator-local ad-hoc signature during the app build:

```text
CODE_SIGNING_ALLOWED=YES
CODE_SIGNING_REQUIRED=NO
CODE_SIGN_IDENTITY="-"
```

- Stop post-build manual deep signing.
- Keep `codesign --verify` and `codesign --display --verbose=4` before install.

Expected next run result:

- If SpringBoard rejected the post-hoc signed bundle, `simctl launch` should now start the shell and emit the required startup markers.
- If launch is still denied, the next artifact should preserve the Xcode-produced code-signing details for a deeper process-launch diagnosis.

### Sixth follow-up: launch step can run too long

Observed progress:

- SDL2 still configured, built, and installed.
- `SorrIOSShell` still configured and built successfully.
- The job reached `Launch iOS shell in simulator`.

Observed failure mode:

- The launch step stayed `in_progress` longer than the previous bounded failure runs.
- That can happen if `simctl launch` remains attached to a successfully started foreground app, or if a simulator launch command hangs before returning a useful exit code.

Fix applied:

- Add a 15-minute timeout to the launch/test step.
- Run each `simctl launch` attempt as a bounded background command.
- If `simctl launch` is still running after 20 seconds, stop that launch command and continue to the existing log-marker checks.
- Preserve the command output in `simulator/simctl-launch-command.log` and the combined launch log.
- Add workflow concurrency so later repair-loop pushes cancel older in-progress iOS shell runs for the same branch.

Expected next run result:

- A successful foreground app launch should no longer hang the workflow; the required shell markers should decide pass/fail.
- A stuck pre-entry launch should fail quickly with uploaded logs rather than waiting for the full job timeout.

### Seventh follow-up: bounded launch still returns exit code 1

Observed public annotation:

```text
Launch iOS shell in simulator
simctl launch failed with exit code 1
```

The build and install steps still completed before the launch failure.

Fix applied:

- Remove `simctl launch --stdout` and `--stderr` redirection from the launch command to avoid any attachment or file-redirection edge case in SpringBoard launch.
- Keep app log capture through simulator `log stream` and `log show`.
- Open `Simulator.app` for the selected UDID after `bootstatus` to make the booted simulator foreground/active before install and launch.
- Emit a compact `::error` annotation with the tail of the launch log and filtered host/system launch logs, so future failures expose the reason without needing artifact download credentials.

Expected next run result:

- If stdout/stderr attachment or a purely headless SpringBoard state caused the denial, the shell should start and emit the required markers.
- If launch is still denied, the public Actions annotations should include the specific SpringBoard/RunningBoard reason for the next patch.

### Eighth follow-up: simulator launch proof passed

Observed result:

```text
Simplify iOS simulator launch diagnostics #13
Commit: 66dcbc9
Status: Success
Total duration: 10m 39s
Artifact: ios-shell-simulator-arm64
```

Because the workflow checks all required startup markers before passing, the successful run proves the shell-only app launches and idles in the simulator.

Working launch shape:

- Xcode builds the simulator app with local ad-hoc signing.
- The selected simulator is booted and foregrounded with `Simulator.app`.
- The app is installed with `simctl install`.
- The app is launched through plain `simctl launch --terminate-running-process` without stdout/stderr file attachment.
- Runtime evidence is captured through simulator unified logging.

No game data was bundled or loaded.

### Ninth iteration: iOS data-layout scaffold, no game load

Goal:

- Resolve the app bundle resource root.
- Resolve `Library/Application Support/SORR` inside the app container.
- Create writable `savegame`, `xbox`, and `logs` directories.
- Write and read a tiny proof file under `logs`.
- Keep `SorR.dat` and all game data out of the bundle and repository.

Implementation under test:

- `SDL_GetBasePath()` supplies the bundle resource root.
- `$HOME/Library/Application Support/SORR` supplies the writable support root.
- The shell creates:
  - `savegame`
  - `xbox`
  - `logs`
- The shell writes `ios_data_layout_probe.txt` under `logs`, reads it back, and logs `data layout test file write/read ok`.

Expected next run result:

- The existing simulator launch proof should remain green.
- The new data-layout markers should pass before the SDL window/renderer and runtime handoff markers.
- `SorR.dat` must still be intentionally not loaded.

Follow-up result:

- The workflow completed successfully on commit `2d3e5a6`.
- The run URL was `https://github.com/Ulises-Estrad/sorr-ios-port/actions/runs/26537165603`.
- The `ios-shell-simulator-arm64` artifact uploaded successfully.
- The marker checks prove the writable Application Support scaffold and write/read proof passed.
- No game data was bundled or loaded.

## Next Step After A Successful Shell Build

After the workflow produces and launches `SorrIOSShell.app` for simulator:

1. Prepare a controlled local/private data bundle or CI artifact input that does not commit `SorR.dat`.
2. Add a read-only bundle root plus writable support-root path shim for game-relative file opens.
3. Generalize the x64-safe runtime fixes from `_WIN64` to a portable 64-bit guard.
4. Build the iOS shell with the x64-safe runtime path.
5. Add the touch keyboard bridge.
6. Only then attempt a private title/city render proof.

Do not bundle or load game data until a later phase explicitly asks for it.

## Device Sideloadly Artifact

The device build job is shell-only and does not install or run on hardware in CI. For D2, it produces a Sideloadly-ready probe IPA that can import private data through iOS file sharing after installation.

Build target:

```text
iphoneos arm64
```

Build signing mode:

```text
CODE_SIGNING_ALLOWED=NO
```

The workflow then applies a CI ad-hoc signature to the `iphoneos` `.app` before IPA packaging. Sideloadly should still re-sign the IPA locally on Windows, but the app bundle inside the IPA is no longer completely unsigned.

Artifact name:

```text
ios-shell-d2-sorr-import-device-arm64
```

Expected IPA:

```text
build-products/SorrIOSShell-d2-sorr-import-adhoc.ipa
```

Expected IPA layout:

```text
Payload/SorrIOSShell.app
```

The job inspects the IPA and fails if it contains:

```text
SorR.dat
data/
*.fpg
*.wav
*.ogg
*.smk
*.png
```

No device signing, provisioning profile, TestFlight, App Store, or paid developer flow is used by CI.

The first successful device artifact run completed on commit `3909ccc`. It uploaded `ios-shell-device-unsigned-arm64` and kept the simulator proof green in the same workflow run.

First physical Sideloadly result:

```text
Sideloadly v0.60
Install failed: Guru Meditation f65043@1006:23a71c Invalid file
IPA: SorrIOSShell-device-unsigned.ipa
```

Current D1 packaging fix result:

- ad-hoc sign the `iphoneos` `.app` with `codesign --sign -` before IPA packaging,
- rename the IPA to `SorrIOSShell-device-adhoc.ipa`,
- unzip the produced IPA into a fresh inspection directory,
- print and validate `Info.plist`,
- validate `CFBundleExecutable`, `CFBundlePackageType`, `CFBundleIdentifier`, `MinimumOSVersion`, `UIDeviceFamily`,
- validate the Mach-O contains `arm64`,
- verify the ad-hoc code signature,
- keep the forbidden asset checks.

Follow-up result:

```text
Commit: 236455a
Run: https://github.com/Ulises-Estrad/sorr-ios-port/actions/runs/26540409093
Result: Success
Artifact: ios-shell-device-unsigned-arm64
Artifact size: 456283 bytes
IPA inside artifact: build-products/SorrIOSShell-device-adhoc.ipa
```

Physical D1 follow-up result:

```text
Install route: Windows + Sideloadly
Artifact: ios-shell-device-unsigned-arm64
IPA: build-products/SorrIOSShell-device-adhoc.ipa
Result: installed on physical iPhone after enabling Developer Mode and trusting the developer profile
Launch result: SorrIOSShell opens to the expected shell-only dark/blank idle screen and remains open
Game data/assets: not bundled
```

This completes D1. Do not proceed to D2 until explicitly requested.

## D2 Data Import Probe

D2 enables these Info.plist keys:

```text
UIFileSharingEnabled = true
LSSupportsOpeningDocumentsInPlace = true
```

The workflow verifies both keys in the packaged IPA.

The IPA remains asset-free. The package inspection still fails if the IPA contains:

```text
SorR.dat
data/
*.fpg
*.wav
*.ogg
*.smk
*.png
```

Manual import route after Sideloadly install:

1. Launch `SorrIOSShell` once to create the iOS app container.
2. On Windows, prepare `SORR_IMPORT` with `SorR.dat`, `mod/system.txt`, `savegame`, `xbox`, and the remaining prepared data.
3. Transfer that folder or a zip of it through iCloud Drive, iCloud.com, OneDrive, Google Drive, or another Files-visible provider.
4. In the iOS Files app, extract the zip if needed.
5. Open `On My iPhone` -> `SorrIOSShell` -> `SORR_IMPORT`.
6. Copy either the prepared data contents or the one extracted top-level folder into that inbox.
7. Return to or relaunch `SorrIOSShell`.
8. Confirm the status screen reports `LAYOUT DIRECT` or `LAYOUT NESTED ONE FOLDER`, `STAGING COPIED`, `SORR.DAT FOUND OPENED`, `MOD/SYSTEM.TXT FOUND`, `PROBE LOG OK`, and writable `savegame`, `xbox`, and `logs`.

The shell treats `Documents/SORR_IMPORT` as an import inbox only. It copies direct or one-folder-nested prepared data into the canonical `Library/Application Support/SORR` root, writes `logs/ios_d2_data_import_probe.txt`, and skips Bennu runtime execution.

Previous GitHub-side D2 artifact proof completed on run `26541978819` with the older `Documents/SORR` inbox.

Updated D2 `SORR_IMPORT` artifact proof:

```text
Commit: 7b395b3
Run: https://github.com/Ulises-Estrad/sorr-ios-port/actions/runs/26543157389
Artifact: ios-shell-d2-sorr-import-device-arm64
Artifact size: 461067 bytes
IPA: build-products/SorrIOSShell-d2-sorr-import-adhoc.ipa
Result: success
```

Physical iPhone D2 result:

```text
Install route: Windows + Sideloadly
Local-only package: out/local-only/SORR_IMPORT.zip
Package source: sorr-vita-master/data
Package SHA256: B2E2F3901C65BE0FA3D739D3716C74C37AE13FB9BDF7535ADC487D9A3D22C72A
Observed status: LAYOUT DIRECT, STAGING COPIED, SORR.DAT FOUND OPENED, MOD/SYSTEM.TXT FOUND, PROBE LOG OK, SAVEGAME WRITABLE, XBOX WRITABLE, LOGS WRITABLE
Game execution: no
Game rendering: no
Game data/assets committed or bundled in IPA: no
```

D3 was later explicitly requested and completed as a separate first-render proof. D2 remains the completed data import/storage milestone and should not be redefined.

## D3 First Render Device Artifact

D3 starts after D1 and D2 are complete. It keeps the simulator job as the shell/data-layout proof and changes the device job into a first-render probe build.

The device configure step now passes:

```text
-DSORR_IOS_D3_FIRST_RENDER=ON
```

Expected D3 artifact:

```text
ios-shell-d3-first-render-device-arm64
```

Expected IPA inside the artifact:

```text
build-products/SorrIOSShell-d3-first-render-adhoc.ipa
```

Current D3 artifact proof:

```text
Commit: d79a53a
Run: https://github.com/Ulises-Estrad/sorr-ios-port/actions/runs/26549967383
Workflow result: success
Device artifact: ios-shell-d3-first-render-device-arm64
Artifact size: 674040 bytes
IPA: build-products/SorrIOSShell-d3-first-render-adhoc.ipa
IPA size: 598798 bytes
Asset inspection: passed, no SorR.dat/data/assets bundled
Physical iPhone result: real SoRR/game runtime rendered and ran from D2-staged data
Audio result: no audible audio yet; later follow-up, not a D3 failure
```

The D3 IPA is still asset-free. The workflow inspection continues to fail if the IPA contains:

```text
SorR.dat
data/
*.fpg
*.wav
*.ogg
*.smk
*.png
```

D3 uses the D2-staged physical-device data at `Library/Application Support/SORR`; it does not upload, bundle, or commit private game data.

Runtime integration changes in the D3 target:

- compile the portable BennuGD runtime source list into the iOS target,
- enable the x64-safe pointer and native handle tables with `SORR_HOST_POINTER_TABLES`,
- keep `mod_sound` stubbed so D3 does not require SDL2_mixer/audio codecs,
- keep PNG loading stubbed for this first render attempt,
- preserve the D2 data preflight before any runtime execution.

First D3 CI failure:

```text
Commit: 8a44a5b
Run: 26545508249
Failed step: Configure iOS shell device build
Cause: CMake 4 rejected vendored tre's cmake_minimum_required(VERSION 3.4)
Fix: the iOS D3 parent CMake now sets CMAKE_POLICY_VERSION_MINIMUM=3.5 before add_subdirectory(tre)
```

Second D3 CI failure:

```text
Commit: 1165d12
Run: 26545953893
Failed step: Build unsigned iOS shell app for device
Cause: strings.c included windows.h after the x64 diagnostic guard was generalized for SORR_HOST_POINTER_TABLES
Fix: Windows-only VirtualQuery diagnostics stay under _WIN64; iOS uses a non-Windows non-null pointer diagnostic fallback
```

Third D3 CI failure:

```text
Commit: 888c629
Run: 26546307412
Failed step: Build unsigned iOS shell app for device
Cause: mod_sys.c uses UIKit Objective-C code when TARGET_IOS is defined but was compiled as C
Fix: enable OBJC language for the D3 iOS project and set mod_sys.c LANGUAGE OBJC only for the D3 Apple target
```

Fourth D3 CI failure:

```text
Commit: be532ad
Run: 26546610123
Failed step: Build unsigned iOS shell app for device
Cause: mod_m7.c passed int callbacks to gr_new_object even though the render object API carries callback context as void *
Fix: mode7 callbacks now accept void * context and convert through intptr_t at the callback boundary
```

Fifth D3 CI failure:

```text
Commit: 362fcb6
Run: 26547029368
Failed step: Build unsigned iOS shell app for device
Cause: mod_flic.c passed FLIC * callbacks to gr_new_object while the render object API expects void * callback context
Fix: FLIC callbacks now accept void * context and cast back to FLIC * inside the callback body
```

Sixth D3 CI failure:

```text
Commit: 1ce7c29
Run: 26547306387
Failed step: Build unsigned iOS shell app for device
Cause: mod_draw.c includes libdraw.h, but the D3 iOS target included mod_draw without adding modules/libdraw to the include path
Fix: add modules/libdraw to the D3 iOS target include directories
```

Seventh D3 CI failure:

```text
Commit: 766ed3d
Run: 26547583863
Failed step: Build unsigned iOS shell app for device
Cause: mod_draw.c passed DRAWING_OBJECT * callbacks to gr_new_object while the render object API expects void * callback context
Fix: mod_draw callbacks now accept void * context and cast back to DRAWING_OBJECT * inside the callback body
```

Eighth D3 CI failure:

```text
Commit: 90596ec
Run: 26547897400
Failed step: Build unsigned iOS shell app for device
Cause: libtext.c passed TEXT * callbacks to gr_new_object while the render object API expects void * callback context
Fix: text callbacks now accept void * context and cast back to TEXT * inside the callback body
```

Ninth D3 CI failure:

```text
Commit: 8f80318
Run: 26548169964
Failed step: Build unsigned iOS shell app for device
Cause: libscroll.c passed int scroll indexes to gr_new_object callbacks while the render object API expects void * callback context
Fix: scroll callbacks now accept void * context and convert through intptr_t at the callback boundary
```

Tenth D3 CI failure:

```text
Commit: 8e5cfa1
Run: 26548410341
Failed step: Build unsigned iOS shell app for device
Cause: libmouse.c passed INSTANCE * callbacks to gr_new_object while the render object API expects void * callback context
Fix: mouse callbacks now accept void * context; the remaining included g_instance render-object callback boundary was converted after a local gr_new_object scan
```

Eleventh D3 CI failure:

```text
Commit: 5597154
Run: 26548706584
Failed step: Build unsigned iOS shell app for device
Cause: interpreter.c included windows.h on iOS after the x64 pointer side table diagnostics were generalized for SORR_HOST_POINTER_TABLES
Fix: keep VirtualQuery diagnostics under _WIN64 and use a non-Windows non-null fallback for SORR_HOST_POINTER_TABLES
```

Twelfth D3 CI failure:

```text
Commit: 1390351
Run: 26549118417
Failed step: Build unsigned iOS shell app for device
Cause: interpreter.c still called Windows GetSystemMetrics in the GET_DESKTOP_SIZE bridge on iOS, and the portable diagnostic guard was closed before its helper functions
Fix: use a 640x480 non-Windows fallback for the bridge and keep the diagnostic helper functions inside the outer PORTABLE_RUNTIME_DIAG guard
```

Thirteenth D3 CI failure:

```text
Commit: 4aa5f00
Run: 26549458571
Failed step: Build unsigned iOS shell app for device
Cause: g_blit.c passed a VERTEX-typed comparator to qsort; AppleClang requires the standard const void * comparator function type
Fix: compare_vertex_y now uses the standard qsort callback ABI and casts to VERTEX internally
```

Fourteenth D3 CI failure:

```text
Commit: 1468e75
Run: 26549773741
Failed step: Build unsigned iOS shell app for device
Cause: dirs.c used GLOB_PERIOD, which is unavailable in the iPhoneOS glob headers
Fix: define GLOB_PERIOD as 0 when the platform does not provide it
```

Fifteenth D3 CI result:

```text
Commit: d79a53a
Run: 26549967383
Result: success
Device artifact: ios-shell-d3-first-render-device-arm64
IPA: build-products/SorrIOSShell-d3-first-render-adhoc.ipa
Asset inspection: passed
```

Physical D3 result:

```text
D3 Actions run: 26549967383
Artifact: ios-shell-d3-first-render-device-arm64
IPA: build-products/SorrIOSShell-d3-first-render-adhoc.ipa
Artifact-producing commit: d79a53a
D3 docs commit before physical proof: 148ead7 [skip ci]
Install route: Windows + Sideloadly
Data source: existing D2-staged Library/Application Support/SORR data
Physical result: actual SoRR game/runtime rendered and the game ran on the physical iPhone
Audio result: no audible audio yet
Game data/assets bundled in IPA: no
Game assets committed: no
D4/D5 status: not started
```

Manual D3 test flow:

1. Keep the D2 data staged on the iPhone.
2. Download the `ios-shell-d3-first-render-device-arm64` artifact.
3. Install `build-products/SorrIOSShell-d3-first-render-adhoc.ipa` with Sideloadly.
4. Open the app.
5. If D2 data is present, the app attempts real SoRR rendering from the staged data.
6. If D2 data is missing, the app displays a visible missing-data status screen.

D3 physical first render proof is complete. Do not proceed to D4 controls, D5 gameplay, or audio follow-up work until explicitly instructed.

## D3 Stability Device Artifact

After D3 first render succeeded on physical iPhone, a repeatable idle exit was observed after about five minutes with the app left untouched. This does not redefine D3 as failed; it is a D3 stability hardening pass before D4a controls.

The device job now publishes a separate stability artifact:

```text
ios-shell-d3-stability-device-arm64
```

IPA inside artifact:

```text
build-products/SorrIOSShell-d3-stability-adhoc.ipa
```

The build remains shell/runtime only and asset-free. It still rejects `SorR.dat`, `data/`, `.fpg`, `.wav`, `.ogg`, `.smk`, and `.png` content during checkout and IPA inspection.

Runtime-side stability diagnostics:

- SDL iOS idle timer disabled before SDL initialization,
- persistent `Library/Application Support/SORR/logs/ios_d3_runtime_stability_probe.txt`,
- previous-run last marker read on next launch,
- 10-second runtime heartbeat,
- current D3 stage in each heartbeat,
- resident memory in each heartbeat when available,
- SDL quit, low-memory, terminating, background, and foreground lifecycle events.

Manual stability test:

1. Keep D2 data staged on the iPhone.
2. Download `ios-shell-d3-stability-device-arm64`.
3. Install `build-products/SorrIOSShell-d3-stability-adhoc.ipa` through Sideloadly.
4. Launch the app and leave it foregrounded and untouched for at least 7 minutes.
5. If it exits, reopen once and report the previous stability marker from the screen/log.

Current D3 stability artifact proof:

```text
Actions run: 26551315103
Device artifact: ios-shell-d3-stability-device-arm64
Artifact size: 675392 bytes
IPA: build-products/SorrIOSShell-d3-stability-adhoc.ipa
Artifact-producing commit: a0a401d
Device job result: success
Game data/assets bundled in IPA: no
```

## D3S Visible Diagnostics Artifact

The first D3 stability artifact still exited after about five minutes, and its private `Application Support` log was not accessible through the iPhone Files app.

The next D3S diagnostic artifact mirrors the same stability log into the app's visible Documents area:

```text
On My iPhone/SorrIOSShell/SORR_DIAGNOSTICS/ios_d3_runtime_stability_probe.txt
```

Expected artifact:

```text
ios-shell-d3s-visible-diagnostics-device-arm64
```

Expected IPA:

```text
build-products/SorrIOSShell-d3s-visible-diagnostics-adhoc.ipa
```

D3S visible diagnostics changes:

- create `Documents/SORR_DIAGNOSTICS`,
- append every D3S stability marker and heartbeat to the visible mirror,
- copy the latest app-private stability log to the visible mirror on startup,
- show `DIAG FILES SORR_DIAGNOSTICS` on the brief pre-runtime status screen,
- keep the D3 render path and D2-staged data path unchanged.

Manual D3S diagnostic test:

1. Keep D2 data staged on the iPhone.
2. Install `build-products/SorrIOSShell-d3s-visible-diagnostics-adhoc.ipa` with Sideloadly.
3. Launch the app and leave it foregrounded and untouched until it exits or 10-15 minutes pass.
4. If it exits, reopen once.
5. Open Files: `On My iPhone -> SorrIOSShell -> SORR_DIAGNOSTICS`.
6. Copy or screenshot the last 20 lines of `ios_d3_runtime_stability_probe.txt`.

Current D3S visible diagnostics artifact proof:

```text
Actions run: 26552137370
Device artifact: ios-shell-d3s-visible-diagnostics-device-arm64
Artifact size: 675260 bytes
IPA: build-products/SorrIOSShell-d3s-visible-diagnostics-adhoc.ipa
Artifact-producing commit: 692c73b
Device job result: success
Game data/assets bundled in IPA: no
```

## D3S Idle-Window Diagnostics Artifact

The visible diagnostics build proved the Files-visible log path, but the app still exited at about five foreground idle minutes. The user confirmed the iPhone is set not to auto-lock, so this pass treats SDL background/terminating lifecycle events as symptoms to log, not as the assumed root cause.

Expected artifact:

```text
ios-shell-d3s-idle-window-device-arm64
```

Expected IPA:

```text
build-products/SorrIOSShell-d3s-idle-window-adhoc.ipa
```

D3S idle-window changes:

- keep the D2 import/staging path unchanged,
- keep the D3 runtime/render path unchanged,
- mirror diagnostics to `Documents/SORR_DIAGNOSTICS`,
- log one-second heartbeats from runtime `240000` ms through `330000` ms,
- log `frame_count`, `last_frame_ticks`, `frame_ms`, FPS counters, skip counters, live instances, render objects, open files, xfile counters, and audio-stub call counters,
- log `dense_window_start`, `dense_window_end`, and `first_frame_detected`.

Manual D3S idle-window test:

1. Keep D2 data staged on the iPhone.
2. Install `build-products/SorrIOSShell-d3s-idle-window-adhoc.ipa` with Sideloadly.
3. Launch the app and leave it foregrounded and untouched until it exits or 10-15 minutes pass.
4. If it exits, reopen once.
5. Open Files: `On My iPhone -> SorrIOSShell -> SORR_DIAGNOSTICS`.
6. Copy or screenshot the last 40-60 lines of `ios_d3_runtime_stability_probe.txt`.
7. Include the `dense_window_start` through `dense_window_end` block when present.

D3S idle-window artifact proof:

```text
Actions run: 26553120903
Device artifact: ios-shell-d3s-idle-window-device-arm64
Artifact size: 677186 bytes
IPA: build-products/SorrIOSShell-d3s-idle-window-adhoc.ipa
IPA size: 601806 bytes
Artifact-producing commit: f6b4347
Device job result: success
Game data/assets bundled in IPA: no
```

## D3A Minimal Audio Stability Artifact

The D3S idle-window diagnostics still pointed at a repeatable foreground exit near five minutes. Memory and file counters were not the obvious cause, while the audio stub counters rose steadily in the 240-300 second window.

The next device artifact therefore pivots from a pure audio stub to a minimal SDL2 audio backend:

- `SOUND_INIT` opens the real SDL audio device on iOS,
- `LOAD_WAV` reads through Bennu `file_open` and decodes WAV through SDL,
- WAV data is converted to the opened device format and queued with `SDL_QueueAudio`,
- unsupported song/music paths are assigned stable inert handles instead of repeated hard failures,
- D3S visible diagnostics remain enabled,
- heartbeat lines include audio init/load/play/inert-handle counters,
- follow-up heartbeat lines include named audio zero categories, live audio handle counts, and last music/BGM file-open status/path.

Artifact target:

```text
ios-shell-d3a-audio-diagnostics-device-arm64
```

IPA target:

```text
build-products/SorrIOSShell-d3a-audio-diagnostics-adhoc.ipa
```

First D3A CI failure:

```text
Run: 26554204340
Failure: AppleClang compile error in sorr_ios_mod_sound_stub.c
Cause: mod_sound globals fixup was used before declaration by GLOEXISTS/GLODWORD macros
Fix: forward-declare __bgdexport(mod_sound, globals_fixup) in the iOS audio replacement
```

D3A GitHub-side artifact proof:

```text
Actions run: 26554585872
Device artifact: ios-shell-d3a-audio-device-arm64
Artifact size: 724145 bytes
IPA: build-products/SorrIOSShell-d3a-audio-adhoc.ipa
Artifact-producing commit: d6785c7
Device job result: success
Simulator job result: success
Game data/assets bundled in IPA: no
```

D3A follow-up diagnostic artifact proof:

```text
Actions run: 26557322892
Device artifact: ios-shell-d3a-audio-diagnostics-device-arm64
Artifact size: 728042 bytes
IPA: build-products/SorrIOSShell-d3a-audio-diagnostics-adhoc.ipa
Artifact-producing commit: 0d3eb53
Device job result: success
Simulator job result: success
Game data/assets bundled in IPA: no
```

Initial physical D3A result:

```text
SFX audio init: ok
WAV loads: audio_wav_load_ok=122 audio_wav_load_fail=0
WAV playback: audio_wav_play=304
audio_stub_minus_one: 0
audio_stub_zero: still climbing
Last marker before exit: heartbeat=49 ticks=274472 runtime_ms=273127
RSS near last marker: approximately 145 MB
Result: still exits around five minutes
```

Manual D3A diagnostic test:

1. Keep the D2-staged data on the iPhone.
2. If the staged data may be missing BGM, regenerate the local-only `out/local-only/SORR_IMPORT.zip` with `tools/create_d2_import_package.ps1`; the helper now verifies `SORR_IMPORT/mod/music/1.ogg`.
3. Install `build-products/SorrIOSShell-d3a-audio-diagnostics-adhoc.ipa` with Sideloadly.
4. Launch `SorrIOSShell` and confirm real SoRR rendering still appears.
5. Note whether audio is audible. WAV effects may work; music may remain silent until SDL2_mixer/OGG/Vorbis is added.
6. Leave the app foregrounded and untouched for 10-15 minutes.
7. If it exits, reopen once and retrieve `On My iPhone/SorrIOSShell/SORR_DIAGNOSTICS/ios_d3_runtime_stability_probe.txt`.
8. Report the last 40-60 lines, including named audio counters, `audio_music_last_*`, and any `SDL_APP_*` lifecycle markers.

## D3A Music/BGM Artifact Target

The next device workflow target upgrades D3A from the previous SFX-only/minimal audio path to a real SDL2_mixer-backed music path:

- fetch SDL2_mixer `2.8.1`,
- build it for `iphoneos` arm64,
- link it into `SorrIOSShell`,
- enable OGG/Vorbis through SDL2_mixer's STB backend,
- keep all game/music data external through the D2 import path,
- keep the IPA asset-free.

Expected artifact:

```text
ios-shell-d3a-music-device-arm64
```

Expected IPA:

```text
build-products/SorrIOSShell-d3a-music-adhoc.ipa
```

Expected CI checks remain:

```text
unzip -l IPA
no SorR.dat
no data/
no .fpg/.wav/.ogg/.smk/.png assets
Info.plist file-sharing keys present
iphoneos arm64 executable
ad-hoc signature valid
```

If the workflow fails, the first likely failure area is SDL2_mixer CMake configuration or static link resolution. The app-side CMake requires `SORR_IOS_SDL2_MIXER_ROOT` for D3 builds because inert/silent BGM is no longer an acceptable D3A endpoint.

First SDL2_mixer D3A CI iteration:

```text
Actions run: 26558533587
Artifact-producing commit: c733c2a
Simulator job result: success
Device job result: failed
Device artifact uploaded: ios-shell-d3a-music-device-arm64 logs only
```

Likely first failure area: SDL2_mixer device configure/static build setup. The follow-up workflow patch forces SDL2_mixer to use a static build with `BUILD_SHARED_LIBS=OFF`, passes the concrete SDL2 CMake package directory through `SDL2_DIR`, and gives the app configure step both SDL2 and SDL2_mixer install prefixes in `CMAKE_PREFIX_PATH`.

D3A music GitHub-side artifact proof:

```text
Actions run: 26559098341
Device artifact: ios-shell-d3a-music-device-arm64
Artifact size: 791638 bytes
IPA: build-products/SorrIOSShell-d3a-music-adhoc.ipa
Artifact-producing commit: 3914295
Device job result: success
Simulator job result: success
Game data/assets bundled in IPA: no
```

Manual D3A music test:

1. Keep the D2-staged data on the iPhone, including `mod/music`.
2. Download `ios-shell-d3a-music-device-arm64` from Actions run `26559098341`.
3. Install `build-products/SorrIOSShell-d3a-music-adhoc.ipa` through Sideloadly.
4. Launch `SorrIOSShell` and confirm real SoRR rendering still appears.
5. Confirm whether BGM is audible.
6. Leave the app foregrounded and untouched for 10-15 minutes.
7. If it exits, reopen once and retrieve `On My iPhone/SorrIOSShell/SORR_DIAGNOSTICS/ios_d3_runtime_stability_probe.txt`.

Physical result from the first SDL2_mixer music artifact:

```text
Tested run: 26559098341
Artifact: ios-shell-d3a-music-device-arm64
IPA: build-products/SorrIOSShell-d3a-music-adhoc.ipa
Observed previous marker: event=SDL_APP_TERMINATING ticks=30496 stage=runtime-loop
Result: early exit, much sooner than the older five-minute idle-window failure
```

The next workflow artifact keeps BGM enabled and adds music lifetime diagnostics. It also changes the music load path from Bennu-backed streaming `SDL_RWops` to an owned in-memory OGG buffer passed to `Mix_LoadMUS_RW`; the buffer stays alive with the music handle until `Mix_FreeMusic`.

Expected follow-up artifact:

```text
ios-shell-d3a-music-diagnostics-device-arm64
build-products/SorrIOSShell-d3a-music-diagnostics-adhoc.ipa
```

D3A music diagnostic GitHub-side artifact proof:

```text
Actions run: 26590593436
Device artifact: ios-shell-d3a-music-diagnostics-device-arm64
Artifact size: 793596 bytes
IPA: build-products/SorrIOSShell-d3a-music-diagnostics-adhoc.ipa
Artifact-producing commit: 2e27dda
Device job result: success
Simulator job result: success
Game data/assets bundled in IPA: no
```

D3A physical music result from that artifact:

```text
BGM: working
SFX: working
Music memory path: working
Music play: working
Old inert music path: gone
Last dense-window marker: heartbeat=46 ticks=271380 runtime_ms=270112
Remaining issue: five-minute foreground exit still occurs
```

The workflow now produces a follow-up D3S runtime-window diagnostic artifact:

```text
Device artifact: ios-shell-d3s-runtime-window-diagnostics-device-arm64
IPA: build-products/SorrIOSShell-d3s-runtime-window-diagnostics-adhoc.ipa
```

This build keeps real BGM/SFX enabled and adds interpreter/runtime process snapshots to the Files-visible D3S stability log. It is meant to identify whether a timed attract/demo/menu process or script state change occurs in the 240-300 second window. The no-assets-in-IPA checks remain unchanged.

GitHub-side D3S runtime-window diagnostic artifact proof:

```text
Actions run: 26592326534
Device artifact: ios-shell-d3s-runtime-window-diagnostics-device-arm64
Artifact size: 796018 bytes
IPA: build-products/SorrIOSShell-d3s-runtime-window-diagnostics-adhoc.ipa
Artifact-producing commit: 7f11519
Device job result: success
Simulator job result: success
Game data/assets bundled in IPA: no
```
## D3S Attract Lifecycle Diagnostic Artifact

The workflow now produces a follow-up D3S device artifact for the remaining five-minute foreground exit:

```text
ios-shell-d3s-attract-lifecycle-diagnostics-device-arm64
```

IPA:

```text
build-products/SorrIOSShell-d3s-attract-lifecycle-diagnostics-adhoc.ipa
```

Purpose:

- keep the D3 first-render path and real BGM/SFX enabled,
- keep the IPA asset-free and use the existing D2-staged private data,
- add process watch counts for the active attract/demo scene processes seen on the physical phone,
- mirror recent process create/destroy lifecycle events into the Files-visible diagnostics log,
- append fatal signal context to `Documents/SORR_DIAGNOSTICS/ios_d3_runtime_stability_probe.txt` when the signal is catchable.

Expected CI checks remain unchanged: build the iphoneos arm64 app, ad-hoc sign it for Sideloadly, package `Payload/SorrIOSShell.app`, inspect Info.plist/architecture/signature, and verify that no `SorR.dat`, `data/`, FPG/WAV/OGG/SMK/PNG assets, logs, or prepared data are present in the IPA.
GitHub-side D3S attract lifecycle artifact proof:

```text
Actions run: 26595939671
Commit: f0553b4
Device artifact: ios-shell-d3s-attract-lifecycle-diagnostics-device-arm64
IPA: build-products/SorrIOSShell-d3s-attract-lifecycle-diagnostics-adhoc.ipa
Artifact size: 800173 bytes
Result: success
```

The simulator shell job also passed in the same run. The device artifact was uploaded after the no-assets-in-IPA inspection and remains suitable for Sideloadly re-sign/install testing.
## D3S Demo Teardown Diagnostic Artifact

The workflow now produces a D3S follow-up artifact aimed at the attract/demo scene cleanup and title/menu re-entry transition:

```text
ios-shell-d3s-demo-teardown-diagnostics-device-arm64
```

IPA:

```text
build-products/SorrIOSShell-d3s-demo-teardown-diagnostics-adhoc.ipa
```

Purpose:

- keep real BGM/SFX enabled,
- keep the IPA asset-free and continue using the existing D2-staged private data,
- expand process lifecycle diagnostics to `FASE1`, `DESCARGA_SISTEMA`, `SISTEMA_SONIDO`, `ASIGNADOR_ENEMIGO`, HUD/effect teardown processes, and title/menu re-entry processes,
- log `destroy_begin` before hierarchy/sibling links are updated and `destroy` after unlink for comparison,
- include family ids/validity and called-by id/validity on lifecycle lines.

Expected CI checks remain unchanged: build iphoneos arm64, ad-hoc sign for Sideloadly, package `Payload/SorrIOSShell.app`, inspect Info.plist/architecture/signature, and verify the IPA contains no `SorR.dat`, `data/`, FPG/WAV/OGG/SMK/PNG assets, logs, or prepared data.

GitHub-side D3S demo teardown artifact proof:

```text
Actions run: 26598228033
Commit: 8017ff7
Device artifact: ios-shell-d3s-demo-teardown-diagnostics-device-arm64
IPA: build-products/SorrIOSShell-d3s-demo-teardown-diagnostics-adhoc.ipa
Artifact size: 798554 bytes
Result: success
```

The simulator shell job also passed in the same run. The device artifact passed the existing IPA layout/signature/forbidden-asset inspection.
## D3S Title Re-Entry Diagnostic Artifact

The workflow now produces a D3S follow-up artifact aimed at the post-demo return-to-title transition:

```text
ios-shell-d3s-title-reentry-diagnostics-device-arm64
```

IPA:

```text
build-products/SorrIOSShell-d3s-title-reentry-diagnostics-adhoc.ipa
```

Purpose:

- keep real BGM/SFX enabled,
- keep the IPA asset-free and continue using the existing D2-staged private data,
- watch `INTRO`, `MENU`, `TROPHIES_CALL`, `TROPHIES_CONTROL`, and `RESOLUCIONX`,
- log `runtime_family_unlink ...` lines with father/son/sibling ids before and after process hierarchy unlinking,
- keep `destroy_begin` and post-unlink `destroy` lifecycle diagnostics for comparison.

Expected CI checks remain unchanged: build iphoneos arm64, ad-hoc sign for Sideloadly, package `Payload/SorrIOSShell.app`, inspect Info.plist/architecture/signature, and verify the IPA contains no `SorR.dat`, `data/`, FPG/WAV/OGG/SMK/PNG assets, logs, or prepared data.


GitHub-side D3S title re-entry artifact proof:

```text
Actions run: 26599649375
Commit: 5d63a6d
Device artifact: ios-shell-d3s-title-reentry-diagnostics-device-arm64
IPA: build-products/SorrIOSShell-d3s-title-reentry-diagnostics-adhoc.ipa
Artifact size: 799257 bytes
Result: success
```

The simulator shell job also passed in the same run. The device artifact passed the existing IPA layout/signature/forbidden-asset inspection.
## D3S LAYER_INTRO Diagnostic Artifact

The workflow now produces a D3S follow-up artifact aimed at the intro/title animation process set seen after demo teardown:

```text
ios-shell-d3s-layer-intro-diagnostics-device-arm64
```

IPA:

```text
build-products/SorrIOSShell-d3s-layer-intro-diagnostics-adhoc.ipa
```

Purpose:

- keep real BGM/SFX enabled,
- keep the IPA asset-free and continue using the existing D2-staged private data,
- watch `LAYER_INTRO` and `INTRO_PRINCIPIO` alongside the existing title re-entry processes,
- log render-object create/destroy counts and watched `runtime_render_event ...` lines,
- log if a render callback fires after its backing `INSTANCE *` is no longer live,
- keep `runtime_family_unlink ...`, `destroy_begin`, and post-unlink `destroy` lifecycle diagnostics.

Expected CI checks remain unchanged: build iphoneos arm64, ad-hoc sign for Sideloadly, package `Payload/SorrIOSShell.app`, inspect Info.plist/architecture/signature, and verify the IPA contains no `SorR.dat`, `data/`, FPG/WAV/OGG/SMK/PNG assets, logs, or prepared data.

GitHub-side D3S LAYER_INTRO artifact proof:

```text
Actions run: 26601081243
Commit: 79342fe
Device artifact: ios-shell-d3s-layer-intro-diagnostics-device-arm64
IPA: build-products/SorrIOSShell-d3s-layer-intro-diagnostics-adhoc.ipa
Artifact size: 799927 bytes
Result: success
```

The simulator shell job also passed in the same run. The device artifact passed the existing IPA layout/signature/forbidden-asset inspection.
## D3S Enemy/HUD Crash Diagnostic Artifact

The workflow now produces a D3S follow-up artifact aimed at the confirmed `SIGSEGV` in the timed attract/demo gameplay path:

```text
ios-shell-d3s-enemy-hud-diagnostics-device-arm64
```

IPA:

```text
build-products/SorrIOSShell-d3s-enemy-hud-diagnostics-adhoc.ipa
```

Purpose:

- keep real BGM/SFX enabled,
- keep the IPA asset-free and continue using the existing D2-staged private data,
- watch `ENEMIGO` and `ESCRIBE_ENEMIGO` alongside the existing enemy/HUD/effect process set,
- include `last_lifecycle`, `last_family`, `last_render`, `last_proc_ptr`, and `current_proc_ptr` in signal-handler output,
- keep render callback guards and watched `runtime_render_event ...` lines.

Expected CI checks remain unchanged: build iphoneos arm64, ad-hoc sign for Sideloadly, package `Payload/SorrIOSShell.app`, inspect Info.plist/architecture/signature, and verify the IPA contains no `SorR.dat`, `data/`, FPG/WAV/OGG/SMK/PNG assets, logs, or prepared data.

GitHub-side D3S enemy/HUD diagnostic artifact proof:

```text
Actions run: 26602412647
Commit: 3009e2d
Device artifact: ios-shell-d3s-enemy-hud-diagnostics-device-arm64
IPA: build-products/SorrIOSShell-d3s-enemy-hud-diagnostics-adhoc.ipa
Artifact size: 800677 bytes
Result: success
```

The simulator shell job also passed in the same run. The device artifact passed the existing IPA layout/signature/forbidden-asset inspection.
