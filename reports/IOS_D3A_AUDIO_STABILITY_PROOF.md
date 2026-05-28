# iOS D3A Audio Stability Proof

Status: D3A audio/stability IPA target prepared.

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
- legacy `audio_stub_zero`
- legacy `audio_stub_minus_one`

The expected improvement is that `audio_stub_minus_one` stops rising. `audio_stub_zero` may still increase for intentional compatibility no-ops such as unsupported music operations.

## Artifact Target

GitHub Actions device artifact:

```text
ios-shell-d3a-audio-device-arm64
```

IPA inside artifact:

```text
build-products/SorrIOSShell-d3a-audio-adhoc.ipa
```

The workflow keeps the no-assets-in-IPA inspection:

```text
no SorR.dat
no data/
no .fpg/.wav/.ogg/.smk/.png assets
```

## Manual iPhone Test

1. Keep the D2-staged data on the iPhone.
2. Download `ios-shell-d3a-audio-device-arm64` from GitHub Actions.
3. Extract the artifact on Windows.
4. Install `build-products/SorrIOSShell-d3a-audio-adhoc.ipa` through Sideloadly.
5. Open `SorrIOSShell`.
6. Confirm the real SoRR render still appears.
7. Note whether any audio is audible.
8. Leave the app foregrounded and untouched for 10-15 minutes.
9. If it exits, reopen it once.
10. Open Files: `On My iPhone -> SorrIOSShell -> SORR_DIAGNOSTICS`.
11. Copy or screenshot the last 40-60 lines of `ios_d3_runtime_stability_probe.txt`.
12. Include any `SDL_APP_*`, `dense_window_start`, `dense_window_end`, and heartbeat lines with the new audio counters.

## Expected Result

Best case: the real SDL audio device opens, WAV effects can be queued, music remains inert or silent, and the app idles 10-15 minutes without exiting.

Diagnostic case: if the app still exits, the visible log should show whether audio initialized, whether WAV loads/plays are happening, whether unsupported inert handles are stable, and whether the previous retry counters still climb near the 240-300 second window.
