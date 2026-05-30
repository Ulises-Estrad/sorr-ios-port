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
Run: https://github.com/Ulises-Estrad/streets-of-rage-remake-ios/actions/runs/26537165603
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

Current immediate action is no active port work. The real game renders and runs on iPhone, BGM/SFX work, touch controls work, custom settings persist, crash reporting remains available, and the app displays as `Streets of Rage`.

```text
Current artifact: ios-shell-playtest-real-point-guard-device-arm64
Current IPA: build-products/SorrIOSShell-playtest-real-point-guard-adhoc.ipa
Control scope: virtual joystick, action buttons, left-side CFG, right-side Start/Back, persisted layout settings
Keep crash reporting and runtime guards
No game data/assets bundled in IPA
```

The project is paused/final for now. Future work should patch only major bugs found during normal playtesting. A known non-blocking issue remains where some no-input attract/demo scenes may ignore Start/Back presses even when the game shows a "press start" prompt; this is not worth pursuing unless it becomes a practical playability blocker.

Historical notes follow.

D1 added a shell-only physical-device artifact before any private data bundle/render proof.

D1 status: physical iPhone Sideloadly install proof complete.

```text
Commit: 3909ccc
Run: https://github.com/Ulises-Estrad/streets-of-rage-remake-ios/actions/runs/26537919700
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
Run: https://github.com/Ulises-Estrad/streets-of-rage-remake-ios/actions/runs/26540409093
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
Run: https://github.com/Ulises-Estrad/streets-of-rage-remake-ios/actions/runs/26541978819
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
Run: https://github.com/Ulises-Estrad/streets-of-rage-remake-ios/actions/runs/26543157389
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
Run: https://github.com/Ulises-Estrad/streets-of-rage-remake-ios/actions/runs/26549967383
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
## D3S Demo Teardown Artifact Proof

The D3S demo teardown diagnostic IPA was produced successfully:

- Actions run: `26598228033`
- Commit: `8017ff7`
- Artifact: `ios-shell-d3s-demo-teardown-diagnostics-device-arm64`
- IPA: `build-products/SorrIOSShell-d3s-demo-teardown-diagnostics-adhoc.ipa`
- Artifact size: `798554` bytes

Next manual step is physical iPhone testing with the existing D2-staged data. D4a remains blocked until this teardown transition log identifies the failure point or narrows the crash to a subsystem outside input.
## D3S Title Re-Entry Follow-Up

The latest D3S phone log indicates the app survives the attract/demo cleanup and then exits during or just after title/intro/trophies/menu re-entry. D3 first render and D3A audio remain achieved; D4a fixed touch input stays blocked while the stability issue is narrowed.

Next diagnostic artifact:

- `ios-shell-d3s-title-reentry-diagnostics-device-arm64`
- `build-products/SorrIOSShell-d3s-title-reentry-diagnostics-adhoc.ipa`

This build keeps BGM/SFX enabled and logs the process family unlink path for the suspected title re-entry processes. The key new marker is `runtime_family_unlink`, which records father/son/sibling ids before and after `instance_destroy` updates hierarchy links. The goal is to determine whether `MENU` teardown leaves `INTRO`, trophies, or resolution helper processes with stale parent/child/called-by relationships.
## D3S Title Re-Entry Artifact Proof

The D3S title re-entry diagnostic IPA was produced successfully:

- Actions run: `26599649375`
- Commit: `5d63a6d`
- Artifact: `ios-shell-d3s-title-reentry-diagnostics-device-arm64`
- IPA: `build-products/SorrIOSShell-d3s-title-reentry-diagnostics-adhoc.ipa`
- Artifact size: `799257` bytes

Next manual step is physical iPhone testing with the existing D2-staged data. D4a remains blocked until this title re-entry log identifies the failure point or narrows the exit to a subsystem outside input.
## D3S LAYER_INTRO Follow-Up

The latest D3S phone log shows title re-entry progressing beyond `MENU` and trophies cleanup, with the final suspicious state around `LAYER_INTRO` and `INTRO_PRINCIPIO`. D3 first render and D3A audio remain achieved; D4a fixed touch input stays blocked while the stability issue is narrowed.

Next diagnostic artifact:

- `ios-shell-d3s-layer-intro-diagnostics-device-arm64`
- `build-products/SorrIOSShell-d3s-layer-intro-diagnostics-adhoc.ipa`

This build keeps BGM/SFX enabled and adds render-object diagnostics for the intro animation path. It logs watched render-object create/destroy events, heartbeat render object create/destroy totals, invalid render-callback guards, and the existing family unlink/lifecycle markers. The goal is to determine whether `LAYER_INTRO`, `INTRO_PRINCIPIO`, or `OSCURECE_PANTALLA` leaves a stale render object or process relationship shortly before termination.
## D3S LAYER_INTRO Artifact Proof

The D3S LAYER_INTRO diagnostic IPA was produced successfully:

- Actions run: `26601081243`
- Commit: `79342fe`
- Artifact: `ios-shell-d3s-layer-intro-diagnostics-device-arm64`
- IPA: `build-products/SorrIOSShell-d3s-layer-intro-diagnostics-adhoc.ipa`
- Artifact size: `799927` bytes

Next manual step is physical iPhone testing with the existing D2-staged data. D4a remains blocked until this intro/title-animation diagnostic log identifies the failure point or narrows the exit to a subsystem outside input.
## D3S Enemy/HUD Crash Follow-Up

The LAYER_INTRO diagnostic build captured a real `SIGSEGV`:

- `signal=11 ticks=275296 stage=runtime-loop`
- last runtime context near `ENEMIGO`, `ESCRIBE_ENEMIGO`, HUD bars, `EFECTO_POLVO`, and `MINI_CUADRO1`
- BGM/SFX still healthy

The next D3S artifact stays in stability diagnostics and does not start D4a input. It adds enemy/HUD-specific watch coverage and signal-time dump fields:

- Artifact target: `ios-shell-d3s-enemy-hud-diagnostics-device-arm64`
- IPA target: `build-products/SorrIOSShell-d3s-enemy-hud-diagnostics-adhoc.ipa`
- New signal fields: `last_lifecycle`, `last_family`, `last_render`, `last_proc_ptr`, `current_proc_ptr`

The goal is to determine whether the crash is a stale process/render reference during enemy/HUD/effect cleanup or creation in the timed attract/demo gameplay path.
## D3S Enemy/HUD Artifact Proof

The D3S enemy/HUD diagnostic IPA was produced successfully:

- Actions run: `26602412647`
- Commit: `3009e2d`
- Artifact: `ios-shell-d3s-enemy-hud-diagnostics-device-arm64`
- IPA: `build-products/SorrIOSShell-d3s-enemy-hud-diagnostics-adhoc.ipa`
- Artifact size: `800677` bytes

Next manual step is physical iPhone testing with the existing D2-staged data. D4a remains blocked until this enemy/HUD signal log identifies the failure point or narrows the crash to a subsystem outside input.
## D3S Enemy/HUD Guard Patch

The latest physical log identified a likely stale enemy/HUD child-process reference after `ESCRIBE_ENEMIGO`, `MINI_CUADRO1`, `BARRA_VIDA1`, and `BARRA_SEC_VIDA1` teardown. D3 first render and D3A audio remain complete; D4a fixed touch input stays blocked while this D3S stability guard is tested.

Next patch artifact:

- `ios-shell-d3s-enemy-hud-guard-device-arm64`
- `build-products/SorrIOSShell-d3s-enemy-hud-guard-adhoc.ipa`

The patch keeps BGM/SFX enabled and keeps the IPA asset-free. It extends the iOS/D3S pointer side table so remote process-local/public pointers carry owner process metadata, then guards later dereferences if the owner process has been destroyed or reused. Guard hits are logged as `runtime_stale_process_ref ...` in the Files-visible diagnostics log.

Success criteria for this patch are unchanged from the D3S stability pass: physical iPhone render works, BGM/SFX work, and the app survives foreground idle past the old five-minute attract/demo crash window. If it still crashes, the visible log should contain stronger breadcrumbs for the next patch attempt.
## D3S Enemy/HUD Guard Artifact Proof

The D3S enemy/HUD guard IPA was produced successfully:

- Actions run: `26603837181`
- Commit: `2eb36b6`
- Artifact: `ios-shell-d3s-enemy-hud-guard-device-arm64`
- IPA: `build-products/SorrIOSShell-d3s-enemy-hud-guard-adhoc.ipa`
- Device job result: success
- Simulator job result: success

Next manual step is physical iPhone testing with the existing D2-staged data. D4a remains blocked until this guard build either survives past the old five-minute window or produces new `runtime_stale_process_ref` / `signal=` breadcrumbs.

## D3S ENEMIGO Lookup Guard Follow-Up

The D3S enemy/HUD remote-pointer guard did not fire before the repeated `SIGSEGV`, so the stability path has shifted to a narrow process-id lookup guard while staying before D4a input work.

Next patch artifact:

- `ios-shell-d3s-enemigo-lookup-guard-device-arm64`
- `build-products/SorrIOSShell-d3s-enemigo-lookup-guard-adhoc.ipa`

The patch keeps BGM/SFX enabled and keeps the IPA asset-free. It validates iOS/D3S `instance_get(id)` candidates before returning them, so a dead hash-slot pointer or an id-mismatched process lookup returns `NULL` instead of a stale process pointer. It also adds a recently-destroyed process ring and visible `runtime_enemigo_lookup_guard ...` diagnostics for ENEMIGO/HUD/effect lookup paths.

D4a remains blocked until this D3S guard build either survives past the old five-minute attract/demo crash window or produces new `runtime_enemigo_lookup_guard` / `signal=` breadcrumbs.

## D3S ENEMIGO Lookup Guard Artifact Proof

The D3S ENEMIGO lookup guard IPA was produced successfully:

- Actions run: `26605317709`
- Patch commit: `562f4f3`
- Artifact: `ios-shell-d3s-enemigo-lookup-guard-device-arm64`
- IPA: `build-products/SorrIOSShell-d3s-enemigo-lookup-guard-adhoc.ipa`
- Artifact size: `803744` bytes
- Device job result: success
- Simulator job result: success

Next manual step is physical iPhone testing with the existing D2-staged data. D4a remains blocked until this guard build either survives past the old five-minute window or produces new `runtime_enemigo_lookup_guard` / `signal=` breadcrumbs.

## D4a Fixed Touch Pass

D4a now proceeds with fixed touch controls while keeping D3S crash hardening active. The previous ENEMIGO lookup guard found a real stale-HUD lookup class, but the remaining crash still needs a cleaner latest-crash report. D4a therefore includes both:

- fixed on-screen D-pad/action/start/back controls,
- compact Files-visible crash reporting.

Artifact target:

```text
ios-shell-d4a-fixed-touch-guarded-device-arm64
build-products/SorrIOSShell-d4a-fixed-touch-guarded-adhoc.ipa
```

The D4a control bridge injects the existing keyboard defaults directly into the iOS `mod_key` path:

| Control | Bennu key |
| --- | --- |
| Up | `72` |
| Down | `80` |
| Left | `75` |
| Right | `77` |
| Attack | `46` |
| Jump | `47` |
| Special | `45` |
| Police | `48` |
| Start/Pause | `28` |
| Back/Menu | `1` plus fallback `14` |

Crash retrieval after a D4a failure:

```text
On My iPhone/SorrIOSShell/SORR_DIAGNOSTICS/ios_latest_crash_report.txt
```

The current-run log remains available at:

```text
On My iPhone/SorrIOSShell/SORR_DIAGNOSTICS/ios_current_run_stability_log.txt
```

D4a still does not start D4b customization, D5 gameplay polish, App Store/TestFlight signing, or asset bundling.

GitHub-side D4a artifact proof:

```text
Actions run: 26607711467
Patch commit: dc64c13
Artifact: ios-shell-d4a-fixed-touch-guarded-device-arm64
IPA: build-products/SorrIOSShell-d4a-fixed-touch-guarded-adhoc.ipa
Artifact size: 811502 bytes
Device job result: success
Simulator job result: success
Game data/assets bundled in IPA: no
```

Physical D4a testing through Sideloadly confirmed that rendering, BGM, SFX, and the fixed touch hitboxes/key injection work. The user could skip intro, navigate, and control gameplay. The old idle crash was not reproduced while actively using touch input, but D3S remains guarded rather than fully closed.

The remaining D4a issue is visual composition only: touch controls are functional but invisible, pillar bars are white, and clipped overlay artifacts appear near the bottom-middle and top-right.

Follow-up artifact target:

```text
ios-shell-d4a-visible-touch-viewport-device-arm64
build-products/SorrIOSShell-d4a-visible-touch-viewport-adhoc.ipa
```

This follow-up preserves the working input mappings and Bennu key injection. It forces black clears around the 16:9 game viewport, resets SDL logical size/viewport/clip/scale before overlay drawing, draws visible translucent controls in full drawable coordinates, and restores renderer state afterward. Do not start D4b customization or D5 until this visible-overlay/viewport artifact is tested.

GitHub-side follow-up artifact proof:

```text
Actions run: 26608727747
Patch commit: ec24f1d
Artifact: ios-shell-d4a-visible-touch-viewport-device-arm64
IPA: build-products/SorrIOSShell-d4a-visible-touch-viewport-adhoc.ipa
Artifact size: 812321 bytes
Device job result: success
Simulator job result: success
Game data/assets bundled in IPA: no
```

Physical testing of the visible-overlay artifact confirmed the app is playable and the overlay is visible. The next D4a refinement keeps BGM/SFX, D3S guards, crash reporting, and the existing action buttons, but replaces independent movement rectangles with a compact single-touch D-pad.

Target:

```text
ios-shell-d4a-dpad-refine-device-arm64
build-products/SorrIOSShell-d4a-dpad-refine-adhoc.ipa
```

The D-pad computes a direction mask from a center point with a deadzone and releases old direction keys before pressing new ones. This should fix slide changes such as Right to Left or Up to Down without changing the action-button path or starting D4b customization.

GitHub-side artifact proof:

```text
Actions run: 26611068603
Patch commit: 95ae6d2
Artifact: ios-shell-d4a-dpad-refine-device-arm64
IPA: build-products/SorrIOSShell-d4a-dpad-refine-adhoc.ipa
Artifact size: 813146 bytes
Device job result: success
Simulator job result: success
Game data/assets bundled in IPA: no
Physical iPhone result: pending manual D-pad slide test
```

## D4b Custom Touch Pass

Physical testing of the compact D-pad artifact showed that movement was still too stiff, with direction changes feeling locked to the initial touch-down. The next pass combines a D4a D-pad feel fix with D4b-lite customization while preserving the working action buttons, Bennu key injection, BGM/SFX, D2 staged-data path, and D3S crash reporting.

Target:

```text
ios-shell-d4b-custom-touch-device-arm64
build-products/SorrIOSShell-d4b-custom-touch-adhoc.ipa
```

Control behavior:

- one active D-pad touch owns movement,
- each finger-motion event recomputes the direction mask from the current point,
- old direction keys release before new direction keys press,
- the deadzone remains centered,
- diagonal zones are intentionally narrower than the previous compact build,
- action buttons and Start/Back keep their working path.

Customization behavior:

- `CFG` enters edit mode,
- drag the D-pad or a button to reposition it,
- `BIG` / `SML` resize the selected control,
- `OPAC` cycles opacity,
- `HIDE` / `SHOW` toggles overlay visibility,
- `RST` restores defaults,
- `DONE` saves and exits,
- settings persist in `On My iPhone/SorrIOSShell/SORR_DIAGNOSTICS/ios_touch_controls.ini`.

This target does not start D5 gameplay polish, D4b advanced customization, App Store/TestFlight signing, or asset bundling.

GitHub-side D4b custom-touch artifact proof:

```text
Actions run: 26612339231
Patch commit: f5beb9a
Artifact: ios-shell-d4b-custom-touch-device-arm64
IPA: build-products/SorrIOSShell-d4b-custom-touch-adhoc.ipa
Artifact size: 798 KB
Device job result: success
Simulator job result: success
Game data/assets bundled in IPA: no
Physical iPhone result: pending manual D-pad/customization test
```

## GitHub Consolidation Baseline

The project is now treated as a playable iPhone port baseline rather than an early proof-only experiment. The current usable baseline is D4b custom touch with the app icon and reinstall-from-scratch docs.

Current status:

- real SoRR renders and runs on physical iPhone,
- BGM works,
- SFX works,
- touch controls work,
- D4b custom controls exist and persist,
- D3S crash reporting and guards remain active,
- the IPA remains asset-free and uses locally imported private data,
- remaining work is normal playtesting, control tuning, polish, and major bug fixes found during real play.

Current artifact target:

```text
ios-shell-d4b-custom-touch-icon-device-arm64
build-products/SorrIOSShell-d4b-custom-touch-icon-adhoc.ipa
```

GitHub-side consolidated icon artifact proof:

```text
Actions run: 26613847549
Commit: 444d0a0
Artifact: ios-shell-d4b-custom-touch-icon-device-arm64
IPA: build-products/SorrIOSShell-d4b-custom-touch-icon-adhoc.ipa
Artifact size: 1955535 bytes
Device job result: success
Simulator job result: success
Game data/assets bundled in IPA: no
```

Next priority after consolidation is to playtest in free time and debug major bugs encountered during real play.

## D4b Joystick Control Baseline

The app-icon build is visually correct, but physical testing showed the compact D-pad still does not feel fluid enough. The next baseline changes only the movement control feel and CFG reliability:

- replace the visible/behavioral D-pad with a virtual joystick,
- keep the same Bennu movement key mapping,
- keep Attack/Jump/Special/Police/Start/Back on their existing working path,
- keep D4b customization and persisted settings,
- keep BGM/SFX enabled,
- keep D3S crash reporting and stale-reference guards,
- keep the IPA asset-free.

Target:

```text
ios-shell-d4b-joystick-controls-icon-device-arm64
build-products/SorrIOSShell-d4b-joystick-controls-icon-adhoc.ipa
```

Joystick behavior:

- one active movement touch owns the joystick,
- finger motion continuously recomputes direction,
- old direction keys release before new direction keys press,
- the center deadzone prevents accidental drift,
- diagonals require intentional off-axis movement,
- `CFG` uses a larger touch target for more reliable edit-mode entry.

This is still a control-scheme finalization pass, not D5 gameplay work or the broader playtest/debugging phase.

GitHub-side joystick-control icon artifact proof:

```text
Actions run: 26617138664
Commit: 64dee98
Artifact: ios-shell-d4b-joystick-controls-icon-device-arm64
IPA: build-products/SorrIOSShell-d4b-joystick-controls-icon-adhoc.ipa
Artifact size: 1956172 bytes
Device job result: success
Simulator job result: success
Game data/assets bundled in IPA: no
Physical iPhone result: pending manual joystick/CFG test
```

## D4b CFG / Toolbar Control Fix

The joystick-control build still needs a config-menu usability fix before moving into the broader playtest/debug loop. Physical testing showed that CFG and DONE only worked from narrow regions, matching an iOS touch-plus-synthetic-mouse duplicate-event problem. The next target suppresses synthetic mouse events after real touch events, moves the edit toolbar away from Start/Back, and replaces the old hide/show overlay control with a gameplay text-label toggle.

Target:

```text
ios-shell-d4b-config-joystick-icon-device-arm64
build-products/SorrIOSShell-d4b-config-joystick-icon-adhoc.ipa
```

Expected behavior:

- CFG opens consistently,
- DONE closes consistently,
- config toolbar appears as a vertical left-side strip,
- Start/Back remain unobstructed,
- gameplay button text is hidden by default but can be toggled with `TXT+` / `TXT-`,
- joystick/action controls, BGM/SFX, D3S diagnostics, and asset-free packaging remain unchanged.

GitHub-side D4b CFG/joystick config artifact proof:

```text
Actions run: 26618164911
Commit: b587ee3
Artifact: ios-shell-d4b-config-joystick-icon-device-arm64
IPA: build-products/SorrIOSShell-d4b-config-joystick-icon-adhoc.ipa
Artifact size: 1956085 bytes
Device job result: success
Simulator job result: success
Game data/assets bundled in IPA: no
Physical iPhone result: pending manual CFG/DONE/toolbar/text-label test
```

## D4b Joystick Release / Pillar CFG Target

The next control target fixes a joystick release regression from the CFG duplicate-event filter and repositions CFG for the iPhone 16 Plus landscape layout. Because the game is 16:9 on a wider iPhone screen, the left pillar area is the safest place for a small utility button. The CFG visual is now small, opaque, and inset from the curved top-left corner.

Target:

```text
ios-shell-d4b-joystick-release-cfg-device-arm64
build-products/SorrIOSShell-d4b-joystick-release-cfg-adhoc.ipa
```

Expected behavior:

- joystick release works even if iOS routes release through the synthetic mouse-up path,
- joystick movement does not remain stuck after lifting,
- CFG is smaller, opaque, and placed in the left pillar-safe band,
- CFG/DONE remain usable,
- Start/Back and gameplay controls remain unobstructed,
- BGM/SFX, D3S diagnostics, app icon, and asset-free packaging remain unchanged.

GitHub-side D4b joystick-release / pillar CFG artifact proof:

```text
Actions run: 26618833984
Commit: e4cbf2b
Artifact: ios-shell-d4b-joystick-release-cfg-device-arm64
IPA: build-products/SorrIOSShell-d4b-joystick-release-cfg-adhoc.ipa
Artifact size: 1956468 bytes
Device job result: success
Simulator job result: success
Game data/assets bundled in IPA: no
App display name: Streets of Rage
Physical iPhone result: pending manual joystick-release/CFG placement test
```

## Current Playtest Controls Baseline

The current roadmap no longer uses proof-stage labels for active work. The iPhone port is treated as a playable baseline: real SoRR renders and runs on physical iPhone, BGM/SFX work, virtual joystick and action controls work, custom control settings persist, crash reporting remains active, and the app displays as `Streets of Rage`.

Current workflow target:

```text
Actions run: 26620604830
Commit: 14eacd2
Artifact: ios-shell-playtest-controls-device-arm64
IPA: build-products/StreetsOfRage-playtest-controls-adhoc.ipa
Build label: ios-playtest-controls
Artifact size: 1957191 bytes
Device job result: success
Simulator job result: success
Game data/assets bundled in IPA: no
```

Current control target:

- small `CFG` button in the left pillar-safe area,
- small `START` button in the right pillar-safe area,
- small `BACK` button directly below `START`,
- virtual joystick movement with the existing Bennu arrow-key mapping,
- persisted custom layout settings,
- drag-required layout movement in edit mode.

## Current Playtest Bug Target: Effects / Water / Gun Guard

The next active work is a normal-playtest crash fix, not a new proof milestone. The gun crash happens when firing, and the Stage 6 crash happens as the beach/water stage starts. The shared suspect is effect/projectile/water process churn around `KEKOS`, `LANZADOR`, `SALPICA_AGUA`, `SANGRE`, `EFECTO_POLVO`, and `SOMBRA`.

Current artifact target:

```text
Actions run: 26623394798
Commit: 20bbdfc
Artifact: ios-shell-playtest-effects-water-gun-guard-device-arm64
IPA: build-products/SorrIOSShell-playtest-effects-water-gun-guard-adhoc.ipa
Build label: ios-playtest-effects-water-gun-guard
Artifact size: 1954581 bytes
Device job result: success
Simulator job result: success
Game data/assets bundled in IPA: no
```

The patch preserves the playable baseline: real render, BGM/SFX, custom controls, icon/name, visible crash reporting, D2-staged private data, and asset-free IPA packaging. The runtime change fixes the iOS/64-bit script pointer table so stale pointer removal does not break later linear-probe lookups during heavy effect churn.

Manual test focus:

1. Pick up a gun and shoot repeatedly.
2. Start Stage 6 and verify the beach/water opening.
3. If it still crashes, retrieve `ios_latest_crash_report.txt` from `SORR_DIAGNOSTICS`.

Follow-up visual regression target:

```text
Actions run: 26624352356
Commit: 4f89f79
Artifact: ios-shell-playtest-effects-water-gun-visual-fix-device-arm64
IPA: build-products/SorrIOSShell-playtest-effects-water-gun-visual-fix-adhoc.ipa
Build label: ios-playtest-effects-water-gun-visual-fix
Artifact size: 1955098 bytes
Device job result: success
Simulator job result: success
Game data/assets bundled in IPA: no
```

Reason: the first guard artifact fired `ptr-adjust-miss` repeatedly on ordinary small script values and zeroed them, causing Stage 6 black rendering and HUD placement/missing-counter problems. The follow-up keeps tombstones and diagnostics, but restores the normal untracked pointer fallback behavior.

## Current Playtest Bug Follow-Up: Water Pointer / Crash Report

Physical testing of the visual-fix artifact restored Stage 6/HUD visuals, but the crash still reproduced during the gun/effect path. The newest report still points at `KEKOS` spawning `SALPICA_AGUA`/`SANGRE`/`EFECTO_POLVO`, so the next patch narrows in on native Chipmunk water/effect pointer handling rather than broad interpreter fallback behavior.

Current artifact target:

```text
Artifact: ios-shell-playtest-water-pointer-crash-report-device-arm64
IPA: build-products/SorrIOSShell-playtest-water-pointer-crash-report-adhoc.ipa
Build label: ios-playtest-water-pointer-crash-report
Actions run: 26625762818
Commit: 93c786e
Artifact size: 1955520 bytes
Device job result: success
Simulator job result: success
Game data/assets bundled in IPA: no
```

Patch contents:

- `DRAW_WATER` and `METABALL` now resolve Bennu pointer parameters through the iOS/64-bit script pointer table.
- Water/effect id lists skip missing instances instead of dereferencing them.
- Chipmunk water emulation resolves bodies through the module body list rather than using truncated `LOC_BODY` pointer cells.
- The Bennu `WaterS` struct is read as script cells on iOS/host-pointer builds, avoiding native arm64 C struct pointer-size layout drift.
- Crash reports now include last native/sysproc call, decoded pointer parameters, last water/effect helper event, runtime counters, and audio counters.

The IPA remains asset-free and keeps the current playable baseline: real render, BGM/SFX, icon/name, joystick/custom controls, D2-staged data, and visible diagnostics.

## Current Playtest Bug Follow-Up: GET_REAL_POINT Guard

The water-pointer/crash-report artifact still reproduced the gun/effect crash. The improved report narrowed the newest signal to `KEKOS` calling `GET_REAL_POINT` with pointer output parameters immediately before the crash, while `last_effect_water=none` showed the water helper was not the final native call.

Current artifact target:

```text
Artifact: ios-shell-playtest-real-point-guard-device-arm64
IPA: build-products/SorrIOSShell-playtest-real-point-guard-adhoc.ipa
Build label: ios-playtest-real-point-guard
```

Patch contents:

- `GET_REAL_POINT` now resolves Bennu pointer output parameters through the iOS/64-bit host pointer table.
- The helper guards null output pointers, missing graphs, bad control-point indices, and undefined control points.
- Crash reports now include `crash_report_version=3`, graph/control-point/output-pointer diagnostics, and a recent native call/return ring.

The playable baseline remains unchanged: real render, BGM/SFX, custom controls, icon/name, D2-staged data, visible diagnostics, and asset-free IPA packaging.

GitHub-side artifact proof:

```text
Actions run: 26672932784
Commit: 3b7811e
Artifact: ios-shell-playtest-run-sfx-back-attack-device-arm64
IPA: build-products/SorrIOSShell-playtest-run-sfx-back-attack-adhoc.ipa
Artifact size: 1961521 bytes
Device job result: success
Simulator job result: success
Game data/assets bundled in IPA: no
```

GitHub-side artifact proof:

```text
Actions run: 26626858854
Commit: cd08c9b
Artifact: ios-shell-playtest-real-point-guard-device-arm64
IPA: build-products/SorrIOSShell-playtest-real-point-guard-adhoc.ipa
Artifact size: 1959448 bytes
Device job result: success
Simulator job result: success
Game data/assets bundled in IPA: no
```

Physical playtest result: the GET_REAL_POINT guard fixed the observed gun-shot crash and Stage 6 beach/water startup crash. Continue with normal playtesting and major bug fixes as they appear.

## Current Playtest Bug Follow-Up: Run SFX / Back Attack

Physical playtesting later found that double-tapping forward/back to run can play the wrong enemy SFX after entering some screens. The issue disappears after restarting the app, which points toward stale or mis-sized audio handle state rather than a missing asset.

Current artifact target:

```text
Artifact: ios-shell-playtest-run-sfx-back-attack-device-arm64
IPA: build-products/SorrIOSShell-playtest-run-sfx-back-attack-adhoc.ipa
Build label: ios-playtest-run-sfx-back-attack
```

Patch contents:

- enables the existing `mod_sound` SDL_mixer handle table for iOS/64-bit host-pointer builds,
- avoids passing `Mix_Chunk *` and `Mix_Music *` through truncated `int` values on iOS arm64,
- keeps the existing real BGM/SFX backend enabled,
- adds circular action-button visuals,
- adds Back Attack above Attack, mapped to Bennu key `57` / Space in that superseded artifact.

The playable baseline remains unchanged: real render, BGM/SFX, custom controls, icon/name, D2-staged data, visible diagnostics, and asset-free IPA packaging.

## Current Playtest Bug Follow-Up: Stable SFX / Edit Controls

Physical testing showed that the first run-SFX/back-attack follow-up did not fully fix the wrong run sound. The next patch keeps the iOS handle table but also treats WAV sample identity as session-stable by filename so stale game-side handles cannot later resolve to a different enemy sample after screen cleanup.

Current artifact target:

```text
Actions run: 26673765318
Commit: 8e926e3
Artifact: ios-shell-playtest-stable-sfx-edit-controls-device-arm64
IPA: build-products/SorrIOSShell-playtest-stable-sfx-edit-controls-adhoc.ipa
Build label: ios-playtest-stable-sfx-edit-controls
Artifact size: 1961595 bytes
Device job result: success
Simulator job result: success
Game data/assets bundled in IPA: no
```

Patch contents:

- reuses existing `LOAD_WAV` handles by path on iOS,
- keeps iOS WAV chunks alive across `UNLOAD_WAV` for the app session,
- preserves real BGM/SFX,
- maps Back Attack to Bennu key `32` / `D`,
- aligns the Back Attack / Attack / Special column closer to Jump / Police,
- lets edit mode deselect a selected control by tapping it again.

The playable baseline remains unchanged: real render, BGM/SFX, custom controls, icon/name, D2-staged data, visible diagnostics, and asset-free IPA packaging.

## Current Playtest Bug Follow-Up: Audio SFX Diagnostics

Physical testing of the stable-SFX/edit-controls artifact still reproduced the wrong dash/run sound after moving through new scenes twice. The important implementation correction is that the iPhone build's live audio path is `sorr_ios_mod_sound_stub.c`; the earlier generic `mod_sound.c` handle-table work was not the active physical-device playback path.

Current artifact target:

```text
Actions run: 26674517695
Commit: adc95b5
Artifact: ios-shell-playtest-audio-sfx-diagnostics-device-arm64
IPA: build-products/SorrIOSShell-playtest-audio-sfx-diagnostics-adhoc.ipa
Build label: ios-playtest-audio-sfx-diagnostics
Artifact size: 1962580 bytes
Device job result: success
Simulator job result: success
Game data/assets bundled in IPA: no
```

Patch contents:

- instruments the actual iOS SDL_mixer WAV backend,
- writes `On My iPhone/Streets of Rage/SORR_DIAGNOSTICS/ios_audio_sfx_diagnostics.txt`,
- logs load/reuse/unload/play events with handle id, serial, sample path, channel, current process, and result,
- keeps WAV handles alive across unloads for the app session and only force-frees them during audio shutdown,
- keeps BGM/SFX, custom controls, icon/name, D2-staged data, visible crash reporting, and asset-free IPA packaging.

Test focus: move through at least two scenes, double-tap forward/back to run, and if the dash sound becomes an enemy SFX, send `ios_audio_sfx_diagnostics.txt` so the exact handle/path mismatch can be traced.

## Current Playtest Bug Follow-Up: Fallback Crash Report

Physical testing of the audio SFX diagnostic artifact suggests the wrong run SFX is no longer reproducing, but a new scene-transition exit did not refresh `ios_latest_crash_report.txt`. That means the exit likely did not reach the signal handler path.

Current artifact target:

```text
Artifact: ios-shell-playtest-fallback-crash-report-device-arm64
IPA: build-products/SorrIOSShell-playtest-fallback-crash-report-adhoc.ipa
Build label: ios-playtest-fallback-crash-report
```

Patch contents:

- appends `clean_shutdown=1` to current-run logs on normal shutdown,
- checks the rotated `ios_previous_run_stability_log.txt` on the next launch,
- if the previous run has no `clean_shutdown=1` and no `signal=`, writes `ios_latest_crash_report.txt` with `crash_report_type=previous-run-nosignal-fallback`,
- includes the previous run tail in the fallback report,
- preserves the iOS SFX diagnostic file, BGM/SFX, custom controls, icon/name, staged-data path, visible diagnostics, and asset-free packaging.

Test focus: if the app exits during a scene transition, reopen once and send `ios_latest_crash_report.txt`, `ios_previous_run_stability_log.txt`, and `ios_current_run_stability_log.txt`.
