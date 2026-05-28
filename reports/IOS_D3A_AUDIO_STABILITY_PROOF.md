# iOS D3A Audio Stability Proof

Status: Initial D3A audio/stability IPA produced and tested on physical iPhone; follow-up audio-category diagnostic IPA produced by GitHub Actions and ready for physical iPhone test.

D3 first render remains complete. D3A is a stability/audio hardening pass for the repeatable foreground idle exit near the five-minute mark.

## Scope

D3A does not start touch controls, gameplay input polish, audio polish, SOR2-only pruning, App Store/TestFlight signing, or bundled game data.

The IPA still relies on the D2-staged data at:

```text
Library/Application Support/SORR
```

No `SorR.dat`, `data/`, FPG/WAV/OGG/SMK/PNG assets, prepared data, local ZIPs, logs, or build outputs are bundled or committed.

## Why This Pass Changed Direction

The idle-window diagnostics showed normal frame timing and no obvious file-handle or memory spike:

```text
dense_window_start ticks=250327 runtime_ms=249045
heartbeat=49 ticks=274425 runtime_ms=273143 stage=runtime-loop
frame_ms=16.667 max_jump=0
opened_files=1 x_files=0 max_x_files=0
rss_bytes approximately 90 MB
```

The suspicious signal was steady growth in the previous audio stub counters:

```text
heartbeat=24 audio_stub_zero=9478 audio_stub_minus_one=235
heartbeat=49 audio_stub_zero=13113 audio_stub_minus_one=308
```

D3A therefore replaces the pure iOS audio stub with a minimal SDL2 audio backend.

## Initial Physical D3A Result

The first D3A audio IPA was installed and tested on the physical iPhone. It still rendered and ran the real SoRR runtime from D2-staged data, and the visible diagnostics confirmed the minimal real SFX path is active:

```text
audio_init_attempts=1
audio_init_ok=1
audio_init_fail=0
audio_wav_load_ok=122
audio_wav_load_fail=0
audio_wav_play=304
audio_stub_minus_one=0
```

The app still exited in the same approximate five-minute foreground idle window:

```text
heartbeat=49 ticks=274472 runtime_ms=273127
rss_bytes approximately 145 MB
```

This means minimal real SFX audio works, but it did not fix the idle exit by itself. The remaining suspicious signals are the still-rising `audio_stub_zero` counter, missing or inert music/BGM behavior, and resource growth around the timed 240-300 second idle/demo window.

## D3A Audio Backend

The iOS `mod_sound` replacement now:

- initializes the real SDL audio subsystem,
- opens a real SDL audio device through the platform audio driver,
- keeps the Bennu `mod_sound` exports and globals intact,
- loads WAV effects through Bennu's `file_open` virtual file layer,
- converts WAV data to the opened SDL audio device format,
- queues WAV effects through `SDL_QueueAudio`,
- returns stable inert handles for unsupported music/OGG paths instead of repeated hard failures,
- uses the existing 64-bit pointer helper for `SP`/`P` sysproc pointer parameters,
- caps queued audio and clears excessive queued bytes as a safety guard,
- keeps Files-visible D3S diagnostics enabled.

This is not full SDL2_mixer/OGG/Vorbis support yet. Music may still be silent. The immediate goal is to stop hammering broken stub paths and test whether foreground idle stability improves.

## New Diagnostics

The visible diagnostics log remains:

```text
On My iPhone/SorrIOSShell/SORR_DIAGNOSTICS/ios_d3_runtime_stability_probe.txt
```

Heartbeat lines now include:

- `audio_init_attempts`
- `audio_init_ok`
- `audio_init_fail`
- `audio_wav_load_ok`
- `audio_wav_load_fail`
- `audio_wav_play`
- `audio_inert_handles`
- `audio_queue_clears`
- `audio_music_load_attempts`
- `audio_music_open_ok`
- `audio_music_open_fail`
- `audio_live_handles`
- `audio_live_wav`
- `audio_live_inert_wav`
- `audio_live_music`
- `audio_max_live_handles`
- `audio_zero_music_play`
- `audio_zero_music_control`
- `audio_zero_music_query`
- `audio_zero_wav_control`
- `audio_zero_wav_query`
- `audio_zero_wav_volume`
- `audio_zero_channel_effect`
- `audio_zero_play_wav_guard`
- `audio_music_last_status`
- `audio_music_last_path`
- legacy `audio_stub_zero`
- legacy `audio_stub_minus_one`

The follow-up diagnostic build splits `audio_stub_zero` into named categories so the next physical run can distinguish music play/control/query calls, WAV control/query/volume calls, channel effects, and guarded `PLAY_WAV` no-ops. It also logs the last music path and whether Bennu's file layer can open it.

## Local-Only Music Import Validation

The local-only Windows helper now validates that the import package includes prepared BGM/music data under `mod/music`. It refuses a source root with no `mod/music` audio files and verifies this ZIP entry in addition to `SorR.dat` and `mod/system.txt`:

```text
SORR_IMPORT/mod/music/1.ogg
```

The regenerated local-only package from `sorr-vita-master/data` contains 237 music/BGM files:

```text
Zip: out/local-only/SORR_IMPORT.zip
SHA256: 261190316E0D539546596738333D6752D69E209FBB8E6B7BF61B0B1DDBAB8A7D
```

This ZIP remains local-only. Do not commit, upload, attach, or bundle it.

## Artifact Target

GitHub Actions device artifact:

```text
ios-shell-d3a-audio-diagnostics-device-arm64
```

IPA inside artifact:

```text
build-products/SorrIOSShell-d3a-audio-diagnostics-adhoc.ipa
```

The workflow keeps the no-assets-in-IPA inspection:

```text
no SorR.dat
no data/
no .fpg/.wav/.ogg/.smk/.png assets
```

## CI Iteration Notes

- Run `26554204340` reached AppleClang for the device build but failed compiling `sorr_ios_mod_sound_stub.c`.
- Cause: the iOS-only audio replacement used Bennu global-access macros before the `mod_sound` globals-fixup array had a visible declaration.
- Fix: add a forward declaration for `__bgdexport(mod_sound, globals_fixup)` so `GLOEXISTS` and `GLODWORD` compile without changing desktop/x64 code.

## GitHub-Side Artifact Proof

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

This is not a physical stability pass yet. It is the installable D3A diagnostic build for the next iPhone test.

## Follow-Up Diagnostic Target

The next D3A diagnostic IPA keeps the same D2 data path and D3 render path but adds named audio zero categories, music file-open diagnostics, and live audio handle counters.

```text
Artifact: ios-shell-d3a-audio-diagnostics-device-arm64
IPA: build-products/SorrIOSShell-d3a-audio-diagnostics-adhoc.ipa
Purpose: identify which remaining audio no-op path climbs near the 240-300 second idle window
Game data/assets bundled in IPA: no
```

GitHub-side follow-up diagnostic artifact proof:

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

## Manual iPhone Test

1. Keep the D2-staged data on the iPhone.
2. If the iPhone data may lack `mod/music`, regenerate the local-only import package on Windows with `powershell -ExecutionPolicy Bypass -File tools\create_d2_import_package.ps1`, then transfer/extract the updated `out/local-only/SORR_IMPORT.zip` through the existing D2 Files route.
3. Download `ios-shell-d3a-audio-diagnostics-device-arm64` from GitHub Actions.
4. Extract the artifact on Windows.
5. Install `build-products/SorrIOSShell-d3a-audio-diagnostics-adhoc.ipa` through Sideloadly.
6. Open `SorrIOSShell`.
7. Confirm the real SoRR render still appears.
8. Note whether any audio is audible.
9. Leave the app foregrounded and untouched for 10-15 minutes.
10. If it exits, reopen it once.
11. Open Files: `On My iPhone -> SorrIOSShell -> SORR_DIAGNOSTICS`.
12. Copy or screenshot the last 40-60 lines of `ios_d3_runtime_stability_probe.txt`.
13. Include any `SDL_APP_*`, `dense_window_start`, `dense_window_end`, and heartbeat lines with the named audio counters and `audio_music_last_*` fields.

## Expected Result

Best case: the real SDL audio device opens, WAV effects can be queued, music remains inert or silent, and the app idles 10-15 minutes without exiting.

Diagnostic case: if the app still exits, the visible log should show whether audio initialized, whether WAV loads/plays are happening, whether unsupported inert handles are stable, and whether the previous retry counters still climb near the 240-300 second window.

## D3A Music Backend Target

The physical D3A diagnostics showed that SFX/WAV playback works, `mod/music` files are present in the local-only import package, BGM path lookup works, and OGG files open through Bennu's file layer. The remaining failure is the iOS music backend itself: the previous `LOAD_SONG` path returned an `open-ok-inert` handle and `PLAY_SONG`/music controls were no-ops.

The next D3A build replaces that inert music path with SDL2_mixer:

- GitHub Actions builds SDL2_mixer for `iphoneos` arm64.
- SDL2_mixer is configured static, vendored, and OGG/Vorbis uses the built-in STB backend.
- The IPA still contains no game data or music assets.
- Runtime BGM stays private through the D2-staged `Library/Application Support/SORR/mod/music` data.
- `LOAD_SONG` now uses `Mix_LoadMUS_RW`.
- `PLAY_SONG`, fade, stop, pause, resume, volume, position, and playing-query calls route to SDL2_mixer.
- WAV/SFX also route through SDL2_mixer so there is one iOS audio backend instead of a queue-audio/SFX path plus inert music.

Artifact target:

```text
ios-shell-d3a-music-device-arm64
```

IPA target:

```text
build-products/SorrIOSShell-d3a-music-adhoc.ipa
```

Expected physical test:

1. Keep the D2-staged data on the iPhone, including `mod/music`.
2. Install `build-products/SorrIOSShell-d3a-music-adhoc.ipa` with Sideloadly.
3. Launch `SorrIOSShell`.
4. Confirm real SoRR rendering still appears.
5. Confirm whether BGM is audible.
6. Leave the app foregrounded and untouched for 10-15 minutes.
7. If it exits, reopen once and retrieve `On My iPhone/SorrIOSShell/SORR_DIAGNOSTICS/ios_d3_runtime_stability_probe.txt`.
8. Report the last 40-60 lines, especially `audio_music_last_status`, `audio_music_last_path`, `audio_music_open_ok`, `audio_music_open_fail`, and any `SDL_APP_*` lifecycle markers.
