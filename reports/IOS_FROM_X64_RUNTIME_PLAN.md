# iOS From X64 Runtime Plan

Date: 2026-05-27

## Updated Status

The old framing of "64-bit pointer ABI is theoretical" is no longer accurate.

Current state:

- x64 desktop runtime builds.
- x64 desktop runtime boots `SorR.dat`.
- x64 reaches character select and live gameplay.
- x64 gameplay timing now matches the practical 32-bit baseline.
- The x64 pointer/handle fixes are the best available base for iOS.

The iOS path should reuse the x64-safe runtime branch instead of starting from the older 32-bit portable assumptions.

## What The X64 Work Proved

The first actual 64-bit failures were concrete and reproducible:

- VM stack cells truncated host addresses.
- File sysprocs returned native `file *` values through 32-bit ints.
- Renderer, palette, sound, and music paths stored native objects through 32-bit-facing values.
- Some string/address opcodes consumed fixed string buffers through truncated stack cells.

The working desktop fixes are:

- x64 pointer side table for stack cells carrying host pointers.
- native file handle table.
- render object handle table.
- palette handle table and renderer palette ID resolution.
- sound/music handle tables.
- targeted x64-safe string/address consumers.
- diagnostic env caching so the x64-safe runtime does not slow gameplay.

This is not yet a complete architectural cleanup, but it is a proven 64-bit compatibility path.

## iOS Base Strategy

Use the x64-safe runtime path as the iOS base.

Do not load `SorR.dat` in the iOS shell until:

- the shell builds on macOS/Xcode,
- SDL2 for iOS is linked,
- the x64-safe runtime changes are included in the iOS target,
- read-only asset paths and writable save/config paths are planned,
- the touch-to-keyboard bridge has a clean injection point.

## Milestone A - Build iOS Shell On Mac Or Cloud Mac

Status: complete on GitHub Actions.

Goal:

- configure the existing iOS shell target,
- link SDL2 for iOS,
- launch on iOS Simulator,
- capture logs.

Expected first success logs:

- app entry reached,
- `SDL_Init` begin/end,
- SDL video init reached,
- SDL window created,
- SDL renderer created,
- `bgdrtm_entry` reached or intentionally stubbed,
- idle loop running.

No `SorR.dat` load yet.

## Milestone B - Add X64-Safe Runtime Changes To iOS Target

Goal:

- make the iOS target compile the same x64-safe runtime path that now works on desktop,
- keep x64 pointer side table and native handle tables enabled for 64-bit Apple builds,
- remove `_WIN64`-only gating where needed and replace it with a shared 64-bit runtime guard.

Likely guard shape:

```c
#if UINTPTR_MAX > UINT32_MAX
```

or a named runtime macro such as:

```c
PORTABLE_RUNTIME_64BIT_HANDLES
```

Important: this should be a focused portability lift of the proven x64 fixes, not a broad VM rewrite.

## Milestone C - iOS Data Layout

Desktop proof assumes a writable working directory. iOS does not have that.

Phase 1 scaffold status: complete on GitHub Actions, no game data.

Proof run:

```text
Commit: 2d3e5a6
Run: https://github.com/Ulises-Estrad/sorr-ios-port/actions/runs/26537165603
Status: Success
```

The shell-only scaffold resolves and logs:

- app bundle resource root,
- `Library/Application Support/SORR`,
- writable `savegame`,
- writable `xbox`,
- writable `logs`.

It also writes and reads a tiny proof file under `logs` to prove Application Support is writable in the simulator container.

Planned layout:

- App bundle:
  - read-only `SorR.dat`,
  - read-only asset folders,
  - read-only `mod`, `data`, `palettes`, audio, FPG, SMK references.
- Application Support or Documents:
  - writable `savegame`,
  - writable config/save overrides,
  - any generated `.sor`, `.cfg`, `.ini`, or screenshot state.

Startup shim:

- locate bundle resource root,
- create writable support root,
- seed missing writable config/save files from bundled defaults,
- chdir or virtualize file opens so existing relative paths resolve correctly.

The first implementation should prefer a small path shim over rewriting game scripts.

## Milestone D - Touch Keyboard Bridge

Use the confirmed keyboard defaults first, not a gamepad abstraction.

Mapping:

| iOS control | Bennu/SDL key |
| --- | --- |
| D-pad Up | Arrow Up |
| D-pad Down | Arrow Down |
| D-pad Left | Arrow Left |
| D-pad Right | Arrow Right |
| Attack | `C` |
| Jump | `V` |
| Special | `X` |
| Police | `B` |
| Start/Pause | Enter |
| Back/Menu | Escape or Backspace |

Preferred bridge design:

- draw a simple SDL/touch overlay,
- translate touch state into virtual keyboard state,
- inject at the portable input layer close to `mod_key`,
- keep the game believing Player 1 is using keyboard defaults.

This avoids the "Gamepad 1 missing" path and matches the proven desktop save/config state.

## Milestone E - SDL2_mixer And Audio Codecs

Add audio only after shell + runtime + data open are proven.

Dependencies:

- SDL2 for iOS,
- SDL2_mixer for iOS,
- zlib,
- libpng,
- OGG,
- Vorbis,
- WAV support through SDL2_mixer or existing runtime paths.

First audio goal:

- initialize mixer,
- load one small WAV/OGG through the same file path shim,
- prove no native audio handle truncation remains on iOS.

## Milestone F - Bundle Prepared Data And Render Title/City

Goal:

- bundle prepared data,
- seed writable save/config,
- launch `SorR.dat`,
- render title/city scene,
- keep event loop responsive,
- prove touch Start reaches character select.

This is the first milestone that should exercise the game data on iOS.

## Immediate Next Action

D1 now adds a shell-only physical-device artifact before any private data bundle/render proof.

D1 status: GitHub Actions artifact proof complete.

```text
Commit: 3909ccc
Run: https://github.com/Ulises-Estrad/sorr-ios-port/actions/runs/26537919700
Artifact: ios-shell-device-unsigned-arm64
Result: success
```

First physical Sideloadly install result:

```text
Sideloadly v0.60
Install failed: Guru Meditation f65043@1006:23a71c Invalid file
```

Current D1 fix under test:

- keep the shell-only `iphoneos` artifact,
- ad-hoc sign the `.app` in CI before IPA packaging,
- produce `SorrIOSShell-device-adhoc.ipa`,
- add deeper IPA validation for `Info.plist`, executable path, `arm64`, and code signature.

Goal:

- build `SorrIOSShell.app` for `iphoneos` `arm64`,
- package it as `Payload/SorrIOSShell.app` inside `SorrIOSShell-device-unsigned.ipa`,
- upload the IPA artifact for Windows download,
- let Sideloadly handle local signing/install on the user's iPhone,
- keep the IPA free of `SorR.dat`, `data/`, and game assets.

Recommended order:

1. Download and install the shell-only IPA with Sideloadly on Windows.
2. Confirm the app opens and reaches the shell idle loop on the physical iPhone.
3. Then decide how to provide prepared game data without committing it.
4. Add a private/local data bundle input or workflow artifact strategy.
5. Add a path shim so read-only bundle assets and writable support-root files can coexist.
6. Lift x64-safe runtime guards from `_WIN64` to a portable 64-bit macro before full game execution on iOS.
7. Attempt first private title/city render proof.

## Out Of Scope For This Plan

- SOR2-only pruning.
- Options submenu parity.
- Full iOS gameplay.
- App Store packaging.
- Store-ready signing/provisioning.
- Broad VM uintptr rewrite unless the x64 side-table strategy hits a clear design blocker.
