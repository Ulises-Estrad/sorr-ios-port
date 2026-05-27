# Repo Upload Checklist

## Staged content

This commit is intended for a private GitHub repo used to preserve the portable runtime work and run the iOS shell workflow. The staged set is limited to:

- Repository hygiene: `.gitignore`
- CI: `.github/workflows/ios-shell.yml`
- Desktop launch helper: `run_sorr_portable.ps1`
- Extraction utility source: `tools/extract_fpg.py`
- Clean Markdown reports under `reports/*.md`
- BennuGD runtime/build source under:
  - `sorr-vita-master/cmake`
  - `sorr-vita-master/core`
  - `sorr-vita-master/modules`
  - `sorr-vita-master/3rdparty`
  - `sorr-vita-master/system.txt`

## Intentionally excluded

Do not commit or upload:

- `SorR.dat`
- `sorr-vita-master/data/`
- extracted/prepared game assets
- `out/`
- `portable-tools/`
- `portable-deps/`
- build directories such as `build-portable*` and `build-ios*`
- binaries such as `.exe` and `.dll`
- media/assets such as `.fpg`, `.wav`, `.ogg`, `.smk`, `.png`
- logs, CSVs, screenshots, cache files, and pycache
- savegame backup blobs

## Why game data/assets are excluded

The repo should contain source, scripts, documentation, and CI configuration only. Streets of Rage Remake game data and extracted assets are not required for the GitHub Actions iOS shell proof, and including them would create unnecessary legal, size, and redistribution risk. The iOS shell workflow is explicitly designed to build an SDL shell without bundling or loading `SorR.dat`.

## Final local checks before push

Run:

```powershell
git diff --cached --name-only
git status --short
```

Confirm the staged list does not include:

```text
SorR.dat
sorr-vita-master/data
out/
portable-tools/
portable-deps/
build-portable
*.exe
*.dll
*.fpg
*.wav
*.ogg
*.smk
*.png
*.pdf
*.log
*.csv
*.pyc
```

## Create the private GitHub repo

1. In GitHub, create a new repository.
2. Set visibility to Private.
3. Do not initialize it with a README, license, or `.gitignore` if pushing this local repo as the initial contents.

GitHub CLI option:

```powershell
gh repo create OWNER/REPO --private --source . --remote origin
```

Manual remote option:

```powershell
git remote add origin git@github.com:OWNER/REPO.git
```

## Push

From the repo root:

```powershell
git push -u origin master
```

If the branch is renamed before pushing:

```powershell
git branch -M main
git push -u origin main
```

## Run the iOS shell workflow

The workflow is `.github/workflows/ios-shell.yml`.

It runs on:

- manual dispatch from the GitHub Actions tab
- pushes that touch the workflow or iOS shell/runtime paths
- pull requests that touch the same paths

Manual run:

1. Open the private repo on GitHub.
2. Go to Actions.
3. Select `iOS Shell`.
4. Click `Run workflow`.
5. Inspect uploaded logs and artifacts after completion.

Expected proof scope:

- macOS runner tool versions are printed.
- iOS simulator SDK is found.
- SDL2 for iOS simulator is fetched/built.
- `SorrIOSShell` configures.
- simulator `.app` builds with signing disabled.
- No game data is uploaded, bundled, or loaded.
