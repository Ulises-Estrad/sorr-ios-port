# macOS/Xcode iOS Build Steps

Date: 2026-05-27

## Scope

This guide is for reproducing the iOS shell milestone on macOS/Xcode only.

Do not load `SorR.dat` in this phase. Do not bundle game data. Do not modify the desktop known-good portable target.

## Required Tools

- Xcode with iOS SDK installed.
- Xcode command line tools selected:

```bash
sudo xcode-select -s /Applications/Xcode.app/Contents/Developer
xcodebuild -version
xcrun --sdk iphonesimulator --show-sdk-path
```

- CMake 3.22 or newer.
- SDL2 built for iOS as either:
  - `SDL2.framework`, or
  - a static/install prefix containing headers and libraries.
- Optional Ninja.

Recommended first attempt: use CMake with the Xcode generator, not Ninja, because this target is an iOS app bundle.

## Get SDL2 For iOS

Option A: use SDL2's Xcode iOS framework/project output.

1. Get SDL2 source from the official SDL project.
2. Open or build the iOS Xcode project included with SDL2.
3. Produce an iOS-compatible `SDL2.framework`.
4. Note the absolute path to that framework.

Example variable used below:

```bash
export SDL2_IOS_FRAMEWORK=/absolute/path/to/SDL2.framework
```

Option B: use an SDL2 iOS install prefix.

The prefix should have a layout like:

```text
/absolute/path/to/sdl2-ios-prefix/
  include/SDL2/SDL.h
  lib/libSDL2.a
```

Example variable:

```bash
export SDL2_IOS_ROOT=/absolute/path/to/sdl2-ios-prefix
```

## Configure With SDL2.framework

From the workspace root:

```bash
cmake -S sorr-vita-master/cmake/ios \
  -B build-ios-shell-sim \
  -G Xcode \
  -DCMAKE_SYSTEM_NAME=iOS \
  -DSORR_IOS_SDL2_FRAMEWORK="$SDL2_IOS_FRAMEWORK"
```

The iOS shell default is:

```text
SORR_IOS_SHELL_USE_SDL_MAIN_HANDLED=OFF
```

Keep that default for the first Xcode attempt. SDL iOS commonly expects SDL's normal main/app entry handling.

## Configure With SDL2 Install Prefix

```bash
cmake -S sorr-vita-master/cmake/ios \
  -B build-ios-shell-sim \
  -G Xcode \
  -DCMAKE_SYSTEM_NAME=iOS \
  -DSORR_IOS_SDL2_ROOT="$SDL2_IOS_ROOT"
```

## Simulator Build Command

List available simulator destinations:

```bash
xcrun simctl list devices available
```

Build for a generic simulator destination:

```bash
cmake --build build-ios-shell-sim \
  --config Debug \
  -- \
  -destination 'generic/platform=iOS Simulator'
```

Build for a named simulator, if needed:

```bash
xcodebuild \
  -project build-ios-shell-sim/sorr_ios_shell.xcodeproj \
  -scheme SorrIOSShell \
  -configuration Debug \
  -destination 'platform=iOS Simulator,name=iPhone 16' \
  build
```

Run from Xcode for the first proof so logs are visible in the console.

## Device Build Command

Device builds require a development team/signing identity. Configure with a bundle identifier that belongs to the team:

```bash
cmake -S sorr-vita-master/cmake/ios \
  -B build-ios-shell-device \
  -G Xcode \
  -DCMAKE_SYSTEM_NAME=iOS \
  -DSORR_IOS_SDL2_FRAMEWORK="$SDL2_IOS_FRAMEWORK" \
  -DSORR_IOS_BUNDLE_IDENTIFIER="com.yourteam.sorr.iosshell"
```

Build:

```bash
cmake --build build-ios-shell-device \
  --config Debug \
  -- \
  -destination 'generic/platform=iOS'
```

If signing fails, open the generated Xcode project and set:

- Team
- Bundle Identifier
- Signing Certificate
- Provisioning Profile, if required

## Expected First Success Log

The first successful launch should show:

```text
SORR iOS shell: app entry
SORR iOS shell: SDL_Init ok
SORR iOS shell: SDL video/events/timer init ok
SORR iOS shell: reached Bennu runtime handoff probe argc=...
SORR iOS shell: bgdrtm_entry returned
SORR iOS shell: SorR.dat intentionally not loaded in milestone 1
SORR iOS shell: entering responsive idle loop
```

Clean quit, if iOS delivers an SDL quit event:

```text
SORR iOS shell: clean shutdown
```

## Shell Runtime Log Checklist

- [ ] app entry reached
- [ ] `SDL_Init` begin/end observed
- [ ] video/events/timer subsystem initialized
- [ ] `SDL_CreateWindow` succeeds
- [ ] `SDL_CreateRenderer` succeeds
- [ ] `bgdrtm_entry` call reached
- [ ] `bgdrtm_entry` returns
- [ ] idle event loop runs
- [ ] clean quit observed if practical

## Common Failure Modes

### CMake cannot find SDL2

Symptom:

```text
SDL2 for iOS was not found
```

Fix:

- Pass `-DSORR_IOS_SDL2_FRAMEWORK=/absolute/path/to/SDL2.framework`, or
- pass `-DSORR_IOS_SDL2_ROOT=/absolute/path/to/sdl2-ios-prefix`.

### SDL2 links but is not embedded

Symptom:

```text
dyld: Library not loaded: @rpath/SDL2.framework/SDL2
```

Fix:

- Prefer `SORR_IOS_SDL2_FRAMEWORK`; the CMake target sets `XCODE_EMBED_FRAMEWORKS`.
- In Xcode, verify SDL2.framework appears under Embed Frameworks.

### App fails before `main`

Symptom:

- No `SORR iOS shell: app entry` log.

Fix:

- Confirm SDL2 iOS main/app delegate integration.
- Retry configure with:

```bash
-DSORR_IOS_SHELL_USE_SDL_MAIN_HANDLED=ON
```

Then inspect whether an explicit SDL-compatible app delegate/main setup is needed.

### Linker complains about Apple frameworks

Symptom:

- Missing UIKit/Foundation/QuartzCore/GameController symbols.

Fix:

- Verify the target is building on Apple/Xcode.
- Confirm the target links:
  - `AudioToolbox`
  - `AVFoundation`
  - `CoreGraphics`
  - `Foundation`
  - `GameController`
  - `QuartzCore`
  - `UIKit`

### ZLIB not found

Symptom:

```text
Could NOT find ZLIB
```

Fix:

- Confirm the iOS SDK is selected and usable.
- If needed, pass a zlib path or use the SDK zlib.

### Code signing fails on device

Symptom:

- Xcode signing/provisioning errors.

Fix:

- Use simulator first.
- For device, set team/signing in Xcode and use a valid bundle identifier.

### Window or renderer creation fails

Symptom:

```text
SDL_CreateWindow failed: ...
SDL_CreateRenderer failed: ...
```

Fix:

- Confirm the SDL2 iOS framework is the same architecture/platform as the build.
- Try simulator first.
- Check whether SDL iOS needs a different main handling mode.

## Stop Condition

Stop this phase once an iOS simulator or device launch reaches:

```text
SORR iOS shell: entering responsive idle loop
```

Do not proceed to `SorR.dat` until this shell proof is documented.

