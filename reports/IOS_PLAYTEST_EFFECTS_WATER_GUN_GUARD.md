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

The first build also tried a broad last-resort guard for untracked script pointer dereferences. Physical testing showed that was too aggressive for normal script math/render fallback paths, so the follow-up visual-fix build restores the normal untracked pointer fallback while keeping the tombstone fix.

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

## Visual Regression Follow-Up

Physical testing of the first guard artifact showed a visual regression:

- Stage 6 rendered completely black.
- The player portrait was shifted left of its slot.
- The lives counter was missing.

The latest crash report also showed `ptr-adjust-miss` guards firing hundreds of thousands of times with small values such as `0x171`. That means the first guard build treated ordinary script pointer/math fallback paths as dead pointers and zeroed them.

Follow-up patch:

- Keep the pointer-table tombstone fix.
- Keep the focused gun/water/effect watchlist.
- Remove the over-broad iOS fallback that zeroed untracked pointer misses.
- Clean up duplicate local declarations in the render module.

Follow-up artifact target:

```text
ios-shell-playtest-effects-water-gun-visual-fix-device-arm64
build-products/SorrIOSShell-playtest-effects-water-gun-visual-fix-adhoc.ipa
```

GitHub-side follow-up artifact proof:

```text
Actions run: 26624352356
Commit: 4f89f79
Artifact: ios-shell-playtest-effects-water-gun-visual-fix-device-arm64
IPA: build-products/SorrIOSShell-playtest-effects-water-gun-visual-fix-adhoc.ipa
Artifact size: 1955098 bytes
Device job result: success
Simulator job result: success
Game data/assets bundled in IPA: no
```

## Water Pointer / Crash Report Follow-Up

Physical testing of the visual-fix artifact confirmed the Stage 6/HUD visuals were restored, but the gun/effect crash still reproduced. The newest crash report still points at the shared effect/water/projectile path:

- `signal=11`
- `current_process=KEKOS`
- `last_lifecycle=create SANGRE`
- `last_render=render_object_create SANGRE`
- recent process churn: `SALPICA_AGUA`, `SANGRE`, `EFECTO_POLVO`

Patch target:

- make `DRAW_WATER`/`METABALL` pointer parameters use the iOS/64-bit script pointer table,
- guard missing process ids in water/effect id lists,
- avoid dereferencing null maps, missing instances, or missing Chipmunk bodies,
- treat Bennu `WaterS` as 32-bit script cells on iOS instead of as a native arm64 C struct,
- keep the previous visual-regression fix,
- keep BGM/SFX, custom controls, app icon/name, and crash reporting.

The latest crash report has also been expanded to include:

- `last_native_call` and decoded raw native/sysproc parameters,
- `last_native_return`,
- `last_effect_water`,
- runtime/render/file counters,
- audio counters,
- the existing lifecycle/family/render rings and current-run tail.

Follow-up artifact target:

```text
ios-shell-playtest-water-pointer-crash-report-device-arm64
build-products/SorrIOSShell-playtest-water-pointer-crash-report-adhoc.ipa
Build label: ios-playtest-water-pointer-crash-report
```

GitHub-side artifact proof:

```text
Actions run: 26625762818
Commit: 93c786e
Artifact: ios-shell-playtest-water-pointer-crash-report-device-arm64
IPA: build-products/SorrIOSShell-playtest-water-pointer-crash-report-adhoc.ipa
Artifact size: 1955520 bytes
Device job result: success
Simulator job result: success
Game data/assets bundled in IPA: no
```

Manual test focus remains:

1. Pick up a gun and shoot repeatedly.
2. Start Stage 6 and verify the beach/water opening.
3. If it crashes, reopen once and send:

```text
On My iPhone/Streets of Rage/SORR_DIAGNOSTICS/ios_latest_crash_report.txt
```

## GET_REAL_POINT Guard Follow-Up

Physical testing of the water-pointer/crash-report artifact still reproduced the gun/effect crash, and the expanded crash report finally narrowed the signal to a native graph-control-point helper:

- `signal=11`
- `current_process=KEKOS`
- `last_native_call=GET_REAL_POINT`
- output pointer parameters decoded through the host pointer table
- recent effect churn still includes `SALPICA_AGUA`, `SANGRE`, and `EFECTO_POLVO`
- `last_effect_water=none`, so the water helper itself was not the final call before the crash

Patch target:

- make `GET_REAL_POINT` resolve output pointer parameters through the iOS/64-bit script pointer table,
- guard null output pointers, missing graphs, bad control-point indices, and undefined control points,
- log the resolved graph/control-point/output-pointer details into the crash report,
- add a recent native call/return ring to `ios_latest_crash_report.txt`,
- keep BGM/SFX, custom controls, app icon/name, visible diagnostics, and asset-free packaging.

Follow-up artifact target:

```text
ios-shell-playtest-real-point-guard-device-arm64
build-products/SorrIOSShell-playtest-real-point-guard-adhoc.ipa
Build label: ios-playtest-real-point-guard
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
