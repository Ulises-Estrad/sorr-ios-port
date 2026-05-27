# iOS Next Blockers

Date: 2026-05-27

## Status

The iOS shell build area exists and passes a local desktop syntax/link check. It has not yet been proven on macOS/Xcode, simulator, or device.

No game data is bundled. `SorR.dat` is not loaded. The desktop known-good target remains untouched.

## Ranked Blockers

### A. Need macOS/Xcode Proof

Priority: highest.

Why:

- This Windows environment cannot run Xcode, the iOS SDK, simulator, or device deployment.
- The shell target is only proven by a local desktop syntax/link check.
- The next real proof is whether the generated app reaches:

```text
SORR iOS shell: app entry
```

Success condition:

- Xcode/simulator/device launch reaches the idle loop log:

```text
SORR iOS shell: entering responsive idle loop
```

### B. SDL2 iOS Dependency Setup

Priority: highest, paired with A.

Why:

- The shell requires SDL2 for iOS.
- The current CMake target accepts either `SDL2.framework` or an SDL2 iOS install prefix, but neither is available in this Windows environment.

Success condition:

- CMake configures with one of:
  - `SORR_IOS_SDL2_FRAMEWORK`
  - `SORR_IOS_SDL2_ROOT`
- App links, launches, and creates an SDL window/renderer.

### C. 64-Bit Pointer ABI Before `SorR.dat`

Priority: blocking before game-data execution.

Why:

- iOS is 64-bit-only.
- Bennu VM/sysproc paths currently pass and return many host pointers through `int`/`uint32_t`.
- `SorR.dat` execution can exercise interpreter pointer ops, file handles, memory handles, sound handles, directory handles, and script pointer params.

Success condition:

- A documented pointer strategy exists before loading `SorR.dat`.
- Likely strategy: pointer-width VM internals plus native handle tables for long-lived host objects.

Do not start broad pointer remediation until the iOS shell build path is proven.

### D. iOS File Layout Before Game Data

Priority: required before bundling/loading prepared data.

Why:

- Desktop proof depends on working directory `sorr-vita-master/data`.
- iOS app bundle is read-only.
- Savegame/config files must be writable in Application Support or Documents.

Success condition:

- Read path plan implemented:
  - writable support root first,
  - app bundle resource root second.
- Write path plan implemented:
  - save/config writes routed to writable support root.
- No bulk asset copy into writable storage.

### E. SDL2_mixer/Audio Later

Priority: after shell and before full playable proof.

Why:

- Current shell does not initialize audio.
- Prepared data includes many `.wav` and `.ogg` files.
- Full runtime needs SDL2_mixer plus OGG/Vorbis support.

Success condition:

- SDL2_mixer built for iOS.
- OGG/Vorbis decoder stack linked.
- Runtime can initialize audio and load representative WAV/OGG assets.

### F. Touch Input Bridge Later

Priority: after shell proof, before playable iOS controls.

Why:

- Desktop playable state relies on keyboard controls.
- iOS should first emulate the confirmed keyboard defaults, not gamepad input.

Success condition:

- Virtual key bridge feeds Bennu key polling:
  - D-pad -> arrow keys
  - Attack -> `C`
  - Jump -> `V`
  - Special -> `X`
  - Police -> `B`
  - Start -> Enter
  - Back -> Escape/Backspace

## Recommended Next Move

Use `reports/MACOS_XCODE_IOS_BUILD_STEPS.md` on a macOS/Xcode machine and document whether the shell reaches:

```text
SORR iOS shell: entering responsive idle loop
```

Only after that should pointer ABI work begin.

