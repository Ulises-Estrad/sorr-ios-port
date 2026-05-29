# iOS Playtest Effects / Water / Gun Guard

## Goal

Patch the first normal-playtest crash class without changing the working iPhone baseline.

Current preserved behavior:

- Real SoRR renders/runs on physical iPhone.
- BGM works.
- SFX works.
- Custom touch controls persist.
- Crash reports remain visible in Files.
- IPA remains asset-free.

## Crash Pattern

Two playtest crashes may share the same runtime path:

- Shooting a gun crashes specifically when the gun fires.
- Stage 6 crashes as the beach/water stage starts.

The gun crash report showed:

- `signal=11`
- `current_process=KEKOS`
- recent effect/projectile/water processes including `SALPICA_AGUA`, `SANGRE`, `EFECTO_POLVO`, `LANZADOR`, and `SOMBRA`

This points at effect/water/projectile process churn rather than controls, audio, or app lifecycle.

## Patch

The iOS 64-bit script pointer table now uses tombstones instead of turning deleted probe slots back into empty slots. This preserves linear-probe chains after stale process-owned pointers are retired, preventing later valid script pointer lookups from falling through to truncated 32-bit addresses.

The patch also keeps a last-resort iOS guard for untracked script pointer dereferences. If a pointer lookup still misses, it is logged as `ptr-get-miss` / `ptr-adjust-miss` and redirected to the existing stale-pointer sink instead of taking a raw SIGSEGV.

Focused diagnostics now watch the gun/water/effect family:

- `KEKOS`
- `LANZADOR`
- `SALPICA_AGUA`
- `SANGRE`
- `EFECTO_POLVO`
- `SOMBRA`
- `FILTRO_RAPIDO`
- `LINEAS_FASE`
- `AGUA`
- `PLAYA`
- `FASE6`

## Artifact Target

```text
ios-shell-playtest-effects-water-gun-guard-device-arm64
build-products/SorrIOSShell-playtest-effects-water-gun-guard-adhoc.ipa
```

GitHub-side artifact proof:

```text
Actions run: 26623394798
Commit: 20bbdfc
Artifact: ios-shell-playtest-effects-water-gun-guard-device-arm64
IPA: build-products/SorrIOSShell-playtest-effects-water-gun-guard-adhoc.ipa
Artifact size: 1954581 bytes
Device job result: success
Simulator job result: success
Game data/assets bundled in IPA: no
```

## Test Plan

1. Install the IPA through Sideloadly.
2. Confirm the existing staged SoRR data still loads.
3. Pick up a gun and shoot repeatedly.
4. Start Stage 6 and verify the beach/water opening.
5. If it crashes, reopen once and send:

```text
On My iPhone/Streets of Rage/SORR_DIAGNOSTICS/ios_latest_crash_report.txt
```
