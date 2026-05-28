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

Current immediate action is D3A audio/stability hardening before D4a touch input.

D3 physical first render is complete, but the rendered app exits after about five foreground idle minutes. The latest dense heartbeat diagnostics did not show an obvious file-handle spike. The first D3A audio pass proved real SDL SFX audio can initialize, load WAV effects, and queue playback, but the app still exited around the same five-minute window. The next build keeps the D2 data path and D3 render path unchanged while adding named audio no-op counters and music/BGM file-open diagnostics.

```text
Artifact target: ios-shell-d3a-audio-diagnostics-device-arm64
IPA target: build-products/SorrIOSShell-d3a-audio-diagnostics-adhoc.ipa
Diagnostics path: On My iPhone/SorrIOSShell/SORR_DIAGNOSTICS/ios_d3_runtime_stability_probe.txt
Audio scope: SDL audio device init, WAV decode/queue through Bennu file_open, inert handles for unsupported music/OGG, named audio zero categories, live audio handle counters, last music/BGM file-open status/path
No D4a touch input yet
No game data/assets bundled in IPA
```

Historical notes follow.

D1 added a shell-only physical-device artifact before any private data bundle/render proof.

D1 status: physical iPhone Sideloadly install proof complete.

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

Current D1 fix result:

- keep the shell-only `iphoneos` artifact,
- ad-hoc sign the `.app` in CI before IPA packaging,
- produce `SorrIOSShell-device-adhoc.ipa`,
- add deeper IPA validation for `Info.plist`, executable path, `arm64`, and code signature.

Ad-hoc-signed follow-up result:

```text
Commit: 236455a
Run: https://github.com/Ulises-Estrad/sorr-ios-port/actions/runs/26540409093
Artifact: ios-shell-device-unsigned-arm64
IPA inside artifact: build-products/SorrIOSShell-device-adhoc.ipa
Result: success
```

Physical iPhone follow-up result:

```text
Physical install route: Windows + Sideloadly
Result: installed after enabling Developer Mode and trusting the developer profile
iPhone behavior: SorrIOSShell opens to a dark/blank shell-only idle screen and remains open
Game data/assets: not bundled
```

D2 status: physical iPhone `SORR_IMPORT` data import/storage proof complete.

D2 does not bundle game data. It enables iOS file sharing so the user can copy prepared data into `Documents/SORR_IMPORT`; the app treats that folder as an import inbox, stages direct or one-folder-nested data into `Library/Application Support/SORR`, and verifies `SorR.dat`, `mod/system.txt`, and writable `savegame`, `xbox`, and `logs` paths.

Previous D2 artifact proof:

```text
Commit: 5d9036f
Run: https://github.com/Ulises-Estrad/sorr-ios-port/actions/runs/26541978819
Artifact: ios-shell-d2-data-import-device-arm64
IPA inside artifact: build-products/SorrIOSShell-d2-data-import-adhoc.ipa
Result: success
```

Updated D2 artifact target:

```text
Artifact: ios-shell-d2-sorr-import-device-arm64
IPA inside artifact: build-products/SorrIOSShell-d2-sorr-import-adhoc.ipa
```

Updated D2 artifact proof:

```text
Commit: 7b395b3
Run: https://github.com/Ulises-Estrad/sorr-ios-port/actions/runs/26543157389
Artifact: ios-shell-d2-sorr-import-device-arm64
Artifact size: 461067 bytes
IPA inside artifact: build-products/SorrIOSShell-d2-sorr-import-adhoc.ipa
Result: success
```

Physical iPhone D2 proof:

```text
Passing Actions run: 26543157389
Artifact: ios-shell-d2-sorr-import-device-arm64
IPA: build-products/SorrIOSShell-d2-sorr-import-adhoc.ipa
Install route: Windows + Sideloadly
Local-only import package: out/local-only/SORR_IMPORT.zip
Local package source: sorr-vita-master/data
Local package SHA256: B2E2F3901C65BE0FA3D739D3716C74C37AE13FB9BDF7535ADC487D9A3D22C72A
Physical result: iPhone D2 probe screen confirmed direct layout, staging copied, SorR.dat opened, mod/system.txt found, savegame/xbox/logs writable, probe log OK
Game execution: no
Game rendering: no
Game data/assets committed or bundled in IPA: no
```

Completed D2 outcome:

- build `SorrIOSShell.app` for `iphoneos` `arm64`,
- package it as `Payload/SorrIOSShell.app` inside `SorrIOSShell-d2-sorr-import-adhoc.ipa`,
- upload the IPA artifact for Windows download,
- let Sideloadly handle local signing/install on the user's iPhone,
- keep the IPA free of `SorR.dat`, `data/`, and game assets.

Completed D2 sequence:

1. Produced the D2 `SORR_IMPORT` file-sharing probe IPA.
2. Installed it manually with Sideloadly.
3. Transferred the prepared `SORR_IMPORT.zip` from Windows.
4. Extracted and copied the prepared data through Files -> `SorrIOSShell` -> `SORR_IMPORT`.
5. Confirmed the app finds/opens `SorR.dat` and `mod/system.txt` and writes `logs/ios_d2_data_import_probe.txt`.
6. D2 is complete; stop until D3 is explicitly requested.
7. Then decide how to move into D3 without committing or uploading private game data.

D3 status: physical iPhone first render proof complete.

D3 keeps the D2 data route and does not bundle game data. The device job builds an asset-free IPA that reads the already staged physical-device data from:

```text
Library/Application Support/SORR
```

D3 build target:

```text
Artifact: ios-shell-d3-first-render-device-arm64
IPA: build-products/SorrIOSShell-d3-first-render-adhoc.ipa
```

D3 GitHub-side artifact proof:

```text
Commit: d79a53a
Run: https://github.com/Ulises-Estrad/sorr-ios-port/actions/runs/26549967383
Workflow result: success
Artifact: ios-shell-d3-first-render-device-arm64
Artifact size: 674040 bytes
IPA: build-products/SorrIOSShell-d3-first-render-adhoc.ipa
IPA size: 598798 bytes
Asset inspection: passed, no SorR.dat/data/assets bundled
Physical iPhone result: real SoRR/game runtime rendered and ran from D2-staged data
Audio result: no audible audio yet; later follow-up, not a D3 failure
```

Physical iPhone D3 proof:

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

D3 implementation strategy:

- keep the D2 preflight and visible status screen,
- require `SorR.dat` and `mod/system.txt` in `Application Support/SORR`,
- enable the x64-safe pointer/handle path for iOS through `SORR_HOST_POINTER_TABLES`,
- compile the portable runtime source list into the iOS target,
- keep audio stubbed and avoid SDL2_mixer for first render,
- attempt the first real title/menu/city render only after D2 data is verified.

D3 remained render-only. Touch controls, gameplay input, audio follow-up, SOR2-only pruning, and App Store/TestFlight signing stay out of scope until explicitly requested.

D3 stability follow-up:

The physical first-render proof is still complete, but a repeatable idle exit was observed after about five minutes. Before D4a touch input, the D3 target now has a stability diagnostic artifact:

```text
ios-shell-d3-stability-device-arm64
build-products/SorrIOSShell-d3-stability-adhoc.ipa
```

This artifact keeps the D2-staged data path, bundles no game data, disables the iOS idle timer through SDL, and writes:

```text
Library/Application Support/SORR/logs/ios_d3_runtime_stability_probe.txt
```

The stability log records runtime stages, 10-second heartbeats, resident memory, lifecycle events, low-memory events, background/foreground transitions, and the previous run's last marker on the next launch.

D3 stability artifact proof:

```text
Actions run: 26551315103
Artifact: ios-shell-d3-stability-device-arm64
Artifact size: 675392 bytes
IPA: build-products/SorrIOSShell-d3-stability-adhoc.ipa
Artifact-producing commit: a0a401d
Device job result: success
Game data/assets bundled in IPA: no
```

D3S visible diagnostics follow-up:

The first stability artifact still exited after about five minutes, and the app-private log was not visible through Files. The D3S diagnostic target now mirrors the stability log to:

```text
Documents/SORR_DIAGNOSTICS/ios_d3_runtime_stability_probe.txt
```

Files path:

```text
On My iPhone/SorrIOSShell/SORR_DIAGNOSTICS/ios_d3_runtime_stability_probe.txt
```

New artifact target:

```text
ios-shell-d3s-visible-diagnostics-device-arm64
build-products/SorrIOSShell-d3s-visible-diagnostics-adhoc.ipa
```

D3S visible diagnostics artifact proof:

```text
Actions run: 26552137370
Artifact: ios-shell-d3s-visible-diagnostics-device-arm64
Artifact size: 675260 bytes
IPA: build-products/SorrIOSShell-d3s-visible-diagnostics-adhoc.ipa
Artifact-producing commit: 692c73b
Device job result: success
Game data/assets bundled in IPA: no
```

D3S idle-window follow-up:

The visible diagnostics build still exits at about five foreground idle minutes. The user confirmed the iPhone is set not to auto-lock. The next artifact keeps D2/D3 behavior unchanged and adds one-second diagnostics from runtime `240000` ms through `330000` ms.

```text
Artifact target: ios-shell-d3s-idle-window-device-arm64
IPA target: build-products/SorrIOSShell-d3s-idle-window-adhoc.ipa
Diagnostics: On My iPhone/SorrIOSShell/SORR_DIAGNOSTICS/ios_d3_runtime_stability_probe.txt
Counters: frame/tick, live instances, render objects, open files, xfiles, audio-stub calls
```

GitHub-side D3S idle-window artifact proof:

```text
Actions run: 26553120903
Artifact: ios-shell-d3s-idle-window-device-arm64
Artifact size: 677186 bytes
IPA: build-products/SorrIOSShell-d3s-idle-window-adhoc.ipa
IPA size: 601806 bytes
Artifact-producing commit: f6b4347
Device job result: success
Game data/assets bundled in IPA: no
```

D3A audio/stability artifact proof:

```text
Actions run: 26554585872
Artifact: ios-shell-d3a-audio-device-arm64
Artifact size: 724145 bytes
IPA: build-products/SorrIOSShell-d3a-audio-adhoc.ipa
Artifact-producing commit: d6785c7
Device job result: success
Simulator job result: success
Game data/assets bundled in IPA: no
Physical idle result: initial D3A audio pass still exited around five minutes
SFX result: audio_init_ok=1, audio_wav_load_ok=122, audio_wav_play=304
Next diagnostic target: ios-shell-d3a-audio-diagnostics-device-arm64
```

The local-only import helper now validates `SORR_IMPORT/mod/music/1.ogg` and found 237 prepared music/BGM files in `sorr-vita-master/data`. If the phone's staged D2 data is stale or missing BGM, refresh it with the regenerated local-only `out/local-only/SORR_IMPORT.zip`; do not commit or upload that ZIP.

D3A follow-up diagnostic artifact proof:

```text
Actions run: 26557322892
Artifact: ios-shell-d3a-audio-diagnostics-device-arm64
Artifact size: 728042 bytes
IPA: build-products/SorrIOSShell-d3a-audio-diagnostics-adhoc.ipa
Artifact-producing commit: 0d3eb53
Device job result: success
Simulator job result: success
Game data/assets bundled in IPA: no
```

## D3A Music/BGM Direction

The physical D3A diagnostics proved that real iOS SFX/WAV audio initializes and plays, but BGM remained `open-ok-inert`: music files were present, path lookup worked, and OGG opened through the file layer, but the iOS music backend returned inert handles and no-op play/control/query results.

The next iOS path therefore uses the x64-safe runtime plus a real SDL2_mixer music backend on iOS:

- build SDL2_mixer for `iphoneos` arm64 in GitHub Actions,
- enable OGG/Vorbis through SDL2_mixer's STB decoder,
- keep private BGM assets in the D2-staged `Library/Application Support/SORR/mod/music` folder,
- keep the IPA free of `SorR.dat`, `data/`, and asset files,
- preserve D3 stability diagnostics in `Documents/SORR_DIAGNOSTICS`.

Target artifact:

```text
ios-shell-d3a-music-device-arm64
build-products/SorrIOSShell-d3a-music-adhoc.ipa
```

GitHub-side D3A music artifact proof:

```text
Actions run: 26559098341
Artifact: ios-shell-d3a-music-device-arm64
Artifact size: 791638 bytes
IPA: build-products/SorrIOSShell-d3a-music-adhoc.ipa
Artifact-producing commit: 3914295
Device job result: success
Simulator job result: success
Game data/assets bundled in IPA: no
```

The next manual physical iPhone test should install this IPA through Sideloadly, keep the D2-staged data with `mod/music` in place, confirm real SoRR rendering still appears, confirm whether BGM is audible, and idle foregrounded for 10-15 minutes while preserving diagnostics in `On My iPhone/SorrIOSShell/SORR_DIAGNOSTICS`.

The first physical SDL2_mixer music IPA exited early, with the previous-run marker showing `event=SDL_APP_TERMINATING ticks=30496 stage=runtime-loop`. This is now tracked as a D3A music-backend crash rather than the older five-minute idle-window issue. The follow-up D3A music diagnostic build keeps BGM enabled, loads OGG music into an owned memory buffer for `Mix_LoadMUS_RW`, keeps that buffer alive with the `Mix_Music` handle, and logs music load/play/control/query/finalization counters in the Files-visible diagnostics.

Follow-up artifact target:

```text
ios-shell-d3a-music-diagnostics-device-arm64
build-products/SorrIOSShell-d3a-music-diagnostics-adhoc.ipa
```

GitHub-side D3A music diagnostic artifact proof:

```text
Actions run: 26590593436
Artifact: ios-shell-d3a-music-diagnostics-device-arm64
Artifact size: 793596 bytes
IPA: build-products/SorrIOSShell-d3a-music-diagnostics-adhoc.ipa
Artifact-producing commit: 2e27dda
Device job result: success
Simulator job result: success
Game data/assets bundled in IPA: no
```

Physical D3A music testing now shows BGM and SFX working from the D2-staged data:

```text
audio_music_mem_ok=7
audio_music_play_ok=6
audio_music_playing=1
audio_music_last_status=play-ok
audio_music_last_path=mod/music/9a.ogg
audio_zero_music_play=0
audio_zero_music_control=0
audio_zero_music_query=0
audio_live_inert_wav=0
```

The remaining blocker is the old foreground idle exit near the 240-300 second window, not inert or missing music. The next D3S artifact therefore keeps BGM/SFX enabled and adds interpreter/runtime snapshots to determine whether an attract/demo/menu process or script state changes immediately before exit:

```text
Artifact target: ios-shell-d3s-runtime-window-diagnostics-device-arm64
IPA target: build-products/SorrIOSShell-d3s-runtime-window-diagnostics-adhoc.ipa
Visible diagnostics: On My iPhone/SorrIOSShell/SORR_DIAGNOSTICS/ios_d3_runtime_stability_probe.txt
```

D4a touch input remains blocked until this D3S runtime-window stability pass has produced a diagnostic result or fix.

GitHub-side D3S runtime-window diagnostic artifact proof:

```text
Actions run: 26592326534
Artifact: ios-shell-d3s-runtime-window-diagnostics-device-arm64
Artifact size: 796018 bytes
IPA: build-products/SorrIOSShell-d3s-runtime-window-diagnostics-adhoc.ipa
Artifact-producing commit: 7f11519
Device job result: success
Simulator job result: success
Game data/assets bundled in IPA: no
```

## Out Of Scope For This Plan

- SOR2-only pruning.
- Options submenu parity.
- Full iOS gameplay.
- App Store packaging.
- Store-ready signing/provisioning.
- Broad VM uintptr rewrite unless the x64 side-table strategy hits a clear design blocker.
## D3S Attract Lifecycle Follow-Up

D3 first render remains complete, and the x64-safe runtime path is still the base for iOS. Latest physical D3S diagnostics show BGM and SFX working, stable file counters, and runtime snapshots entering an active attract/demo/gameplay-like scene around the old 240-300 second exit window.

The next D3S build does not start D4a controls. It keeps BGM/SFX enabled and adds process lifecycle diagnostics for the currently suspected timed runtime path:

- Artifact target: `ios-shell-d3s-attract-lifecycle-diagnostics-device-arm64`
- IPA target: `build-products/SorrIOSShell-d3s-attract-lifecycle-diagnostics-adhoc.ipa`
- Log path: `Documents/SORR_DIAGNOSTICS/ios_d3_runtime_stability_probe.txt`

The intended output is a clearer answer to whether the exit occurs during a specific attract/demo transition, process create/destroy burst, invalid process state, or catchable fatal signal. D4a fixed touch input should stay blocked until this D3S pass produces a result or a narrow fix.
## D3S Attract Lifecycle Artifact Proof

The D3S attract lifecycle diagnostic IPA was produced successfully:

- Actions run: `26595939671`
- Commit: `f0553b4`
- Artifact: `ios-shell-d3s-attract-lifecycle-diagnostics-device-arm64`
- IPA: `build-products/SorrIOSShell-d3s-attract-lifecycle-diagnostics-adhoc.ipa`
- Artifact size: `800173` bytes

Next manual step is physical iPhone testing with the existing D2-staged data. D4a fixed touch input remains blocked until the D3S idle exit is understood or narrowed to a non-control subsystem.
## D3S Demo Teardown Follow-Up

Latest physical D3S diagnostics point at an attract/demo gameplay cleanup and title/menu re-entry transition rather than passive idle or audio failure. D3 first render and D3A audio remain achieved; D4a touch input stays blocked while this D3S stability issue is narrowed.

Next diagnostic artifact:

- `ios-shell-d3s-demo-teardown-diagnostics-device-arm64`
- `build-products/SorrIOSShell-d3s-demo-teardown-diagnostics-adhoc.ipa`

This build keeps BGM/SFX enabled and instruments process lifecycle around `FASE1`, `DESCARGA_SISTEMA`, `SISTEMA_SONIDO`, `ASIGNADOR_ENEMIGO`, HUD/effect teardown, and title/menu re-entry processes. It compares `destroy_begin` state before hierarchy updates against post-unlink `destroy` state to look for stale process ids, caller pointers, sibling links, or a cleanup burst immediately before exit.
