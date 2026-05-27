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

Status: CI build proof successful; simulator launch proof still pending.

GitHub Actions result:

```text
Build iOS shell for simulator arm64: succeeded in 4m 22s
```

The successful macOS CI run completed:

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

This proves:

- GitHub Actions macOS/Xcode runner works,
- iPhoneSimulator SDK is available,
- SDL2 2.30.12 can be built and installed for `iphonesimulator` `arm64`,
- `SorrIOSShell` configures against the SDL2 iOS install,
- static SDL2's required Apple framework closure is linked,
- simulator `.app` build and artifact upload succeed.

Next proof now wired in CI:

- boot an available iPhone simulator,
- print app bundle metadata,
- ad-hoc sign the built simulator app for local `simctl` launch,
- install `SorrIOSShell.app`,
- launch it,
- capture app stdout/stderr, app logs, and broader system launch logs,
- fail the job if required shell startup markers are missing.

Still pending until the updated workflow run completes:

- installing/running the `.app` in an actual simulator,
- runtime log capture from app launch,
- SDL window/renderer creation at runtime,
- responsive idle loop on simulator.

## Earlier Local Blocker

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

Successful in GitHub Actions.

The local Windows session still cannot run the simulator build directly, but CI built the iOS simulator shell app on macOS.

Expected artifact:

```text
ios-shell-simulator-arm64
build-products/SorrIOSShell-iphonesimulator-arm64.zip
```

## Simulator Launch

Workflow step added; run pending.

The GitHub Actions workflow now adds `Launch iOS shell in simulator` after the build step and before artifact packaging. It:

- lists available simulator devices,
- selects the first available iPhone simulator,
- boots and waits for the simulator,
- prints `Info.plist`, bundle id, and launch storyboard diagnostics,
- prints supported orientations and bundle file diagnostics,
- applies a local simulator-only ad-hoc signature with `codesign -`,
- installs the built app,
- launches the bundle id from the app `Info.plist`,
- redirects app stdout/stderr to artifact logs,
- streams logs while the shell runs,
- also captures a `log show --last 2m` fallback,
- captures broader SpringBoard/FrontBoard/RunningBoard/LaunchServices logs,
- combines launch/log output into `simulator/sorr-ios-shell-combined.log`.

## Runtime Log Checklist

- [x] iOS simulator `.app` built: proven by GitHub Actions
- [x] artifact packaging/upload: proven by GitHub Actions
- [ ] app entry reached: workflow check added, pending run
- [ ] `SDL_Init` begin/end: workflow check added, pending run
- [ ] SDL video init: workflow check added, pending run
- [ ] `SDL_CreateWindow` success: workflow check added, pending run
- [ ] `SDL_CreateRenderer` success: workflow check added, pending run
- [ ] `bgdrtm_entry` reached: workflow check added, pending run
- [ ] idle loop running: workflow check added, pending run
- [ ] clean quit: not proven

## First Launch Failure

The first simulator launch attempt reached simulator boot/install, then failed at `simctl launch`:

```text
An error was encountered processing the command (domain=FBSOpenApplicationServiceErrorDomain, code=1):
Simulator device failed to launch dev.local.sorr.iosshell.ci.
The request was denied by service delegate (SBMainWorkspace).
```

Current fix:

- keep Xcode build signing disabled,
- ad-hoc sign the built simulator `.app` locally with `codesign --sign -`,
- verify the signature before `simctl install`.

This is simulator-local signing only. It does not start device signing, IPA export, TestFlight, or App Store work.

Follow-up result:

- Ad-hoc signing succeeded and verified.
- `simctl launch` still failed with the same `SBMainWorkspace` denial.

Second fix:

- add a minimal `LaunchScreen.storyboard`,
- set `UILaunchStoryboardName` to `LaunchScreen`,
- include the storyboard in the simulator app bundle,
- print `Info.plist` diagnostics in CI,
- continue collecting simulator logs even if `simctl launch` fails.

Second follow-up result:

- `UILaunchStoryboardName=LaunchScreen` was present.
- The simulator app installed and had a valid ad-hoc signature.
- `simctl launch` still failed before the process started.
- The narrow app log filter did not reveal the lower-level reason.

Third fix:

- make the shell app iPhone-only for the simulator proof,
- add `CFBundleDisplayName`,
- include portrait plus landscape orientations,
- add `UIApplicationSupportsIndirectInputEvents`,
- hide the status bar,
- capture broader SpringBoard/FrontBoard/RunningBoard/LaunchServices diagnostics.

Third follow-up result:

- `LaunchScreen.storyboardc` was present in `SorrIOSShell.app`.
- `Info.plist` had `UIDeviceFamily=1`, `CFBundleDisplayName`, portrait plus landscape orientations, and `UILaunchStoryboardName=LaunchScreen`.
- The app installed and `simctl listapps` returned the bundle metadata.
- `simctl launch` still failed before app entry with `FBSOpenApplicationServiceErrorDomain code=1` and `SBMainWorkspace`.
- No shell process logs appeared, confirming the failure is still pre-entry.

Fourth fix:

- remove the storyboard launch-screen dependency from the bundle target,
- replace `UILaunchStoryboardName=LaunchScreen` with an empty `UILaunchScreen` dictionary,
- print the full app bundle tree in CI,
- add explicit `SDL_CreateWindow success` and `SDL_CreateRenderer success` shell log markers,
- retry `simctl launch` up to four times with short backoff after install,
- capture host-side CoreSimulator logs in addition to in-simulator SpringBoard/FrontBoard logs.

Fourth follow-up result:

- `SorrIOSShell.app` contained only the executable, `Info.plist`, and `PkgInfo`; no missing `LaunchScreen.storyboardc` remained.
- `Info.plist` contained `UILaunchScreen = {}`.
- The app installed and `simctl listapps` reported the final bundle path.
- `simctl launch` still failed before app entry with `FBSOpenApplicationServiceErrorDomain code=1` and `SBMainWorkspace`.
- Host-side logs showed LaunchServices/RunningBoard treating the app identity as invalid during install coordination and applying a prevent-launch limitation before the shell process started.

Fifth fix:

- reset the selected simulator with `simctl shutdown` plus `simctl erase` before boot,
- explicitly uninstall the bundle id before each install attempt,
- keep the launch retries and host-side CoreSimulator diagnostics,
- make `CFBundleDisplayName` match `SorrIOSShell` to avoid a spaced placeholder app name during install coordination.

Fifth follow-up result:

- The erased simulator removed the stale placeholder symptom.
- LaunchServices registered the real app and `simctl listapps` showed final metadata for `dev.local.sorr.iosshell.ci`.
- `simctl launch` still failed before app entry.
- Host-side logs narrowed the failure to a process-launch denial:

```text
FBSOpenApplicationServiceErrorDomain code=1
FBProcessExit Code=64 "The process failed to launch."
RBSRequestErrorDomain Code=5 "Launch failed."
```

Sixth fix:

- Build the simulator `.app` with Xcode's local ad-hoc signing path:

```text
CODE_SIGNING_ALLOWED=YES
CODE_SIGNING_REQUIRED=NO
CODE_SIGN_IDENTITY="-"
```

- Stop manually deep-signing the app bundle after the Xcode build.
- Verify and print the Xcode-produced signature before simulator install.

This remains simulator-only. It does not use provisioning profiles, create an IPA, install on a device, or start TestFlight/App Store signing.

Sixth follow-up:

- The signed shell build still completed and the job reached the simulator launch step.
- The launch step then remained active longer than the previous failure runs.
- This suggests either a stuck `simctl launch` command or a successful foreground launch where `simctl launch` stays attached instead of returning promptly.

Seventh fix:

- Add a 15-minute timeout to the launch/test step.
- Bound each `simctl launch` attempt to 20 seconds.
- If the launch command remains attached after that window, stop the launch command and continue to the existing simulator log-marker checks.
- Add `simulator/simctl-launch-command.log` to preserve the raw command output from the bounded launch attempt.
- Add workflow concurrency so a newer repair-loop push cancels any older in-progress iOS shell run on the same branch.

Seventh follow-up:

- The launch step no longer ran indefinitely.
- Public GitHub annotations still reported `simctl launch failed with exit code 1`.
- Full artifact logs remain authentication-gated from this local machine.

Eighth fix:

- Remove `--stdout` and `--stderr` from `simctl launch`; keep log capture through simulator unified logging.
- Open `Simulator.app` for the selected simulator after `bootstatus` to avoid a purely headless SpringBoard launch attempt.
- Add `simulator/launch-failure-summary.log`.
- Emit a compact GitHub `::error` annotation containing launch-log and filtered SpringBoard/RunningBoard/CoreSimulator failure tails for future no-auth inspection.

## Expected First Success Log

When run from a real macOS/Xcode+iOS simulator environment, the target should still be validated against:

```text
SORR iOS shell: app entry
SORR iOS shell: SDL_Init ok
SORR iOS shell: SDL video/events/timer init ok
SORR iOS shell: SDL_CreateWindow success
SORR iOS shell: SDL_CreateRenderer success
SORR iOS shell: reached Bennu runtime handoff probe argc=...
SORR iOS shell: bgdrtm_entry returned
SORR iOS shell: SorR.dat intentionally not loaded in milestone 1
SORR iOS shell: entering responsive idle loop
```

## Next Required Action

Run the updated GitHub Actions workflow. The launch step should capture logs until the shell reaches:

```text
SORR iOS shell: entering responsive idle loop
```

Expected uploaded launch logs:

```text
ci-artifacts/ios-shell/logs/ios-shell-simulator-launch.log
ci-artifacts/ios-shell/logs/simulator/simctl-launch.log
ci-artifacts/ios-shell/logs/simulator/sorr-ios-shell-stdout.log
ci-artifacts/ios-shell/logs/simulator/sorr-ios-shell-stderr.log
ci-artifacts/ios-shell/logs/simulator/sorr-ios-shell-log-stream.log
ci-artifacts/ios-shell/logs/simulator/sorr-ios-shell-log-show.log
ci-artifacts/ios-shell/logs/simulator/sorr-ios-shell-system-log-show.log
ci-artifacts/ios-shell/logs/simulator/sorr-ios-shell-host-coresimulator-log-show.log
ci-artifacts/ios-shell/logs/simulator/sorr-ios-shell-combined.log
```
