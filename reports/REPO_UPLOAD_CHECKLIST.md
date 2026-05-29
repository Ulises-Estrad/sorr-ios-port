# Repo Upload Checklist

## Current Policy

The repository is now intended to be self-contained for recovery. Source, scripts, docs, iOS build configuration, the app icon, and the prepared SoRR data root are kept on GitHub.

Large/binary game data is stored through Git LFS. Generated outputs remain ignored.

## Tracked Through Git LFS

- `sorr-vita-master/data/**`
- SoRR/Bennu media files under `sorr-vita-master`, including FPG/WAV/OGG/SMK/PNG/PDF/DIA-style binary assets
- archive/binary formats if intentionally added later

## Still Excluded

Do not add generated local junk unless there is a specific reason:

- `out/`
- `out/local-only/SORR_IMPORT.zip`
- `portable-tools/`
- `portable-deps/`
- build directories such as `build-portable*` and `build-ios*`
- generated IPAs/apps/dSYMs/xcresults
- logs, CSVs, screenshots, cache files, and pycache

## IPA Rule

The repo may contain Git LFS-managed SoRR data, but the GitHub Actions IPA remains data-free. The packaging step must continue to fail if `Payload/SorrIOSShell.app` contains:

```text
SorR.dat
data/
*.fpg
*.wav
*.ogg
*.smk
non-icon *.png
logs
prepared import zips
```

The iPhone app still receives data through the Files import/staging flow.

## Final Checks Before Push

Run:

```powershell
git status --short
git lfs ls-files
git diff --cached --name-only
```

Confirm generated outputs are not staged unless deliberately needed.
