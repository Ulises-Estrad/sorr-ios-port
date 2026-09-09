"""Stage and verify the complete LFS-resolved game data in an iOS app/IPA."""
import argparse
import hashlib
import json
from pathlib import Path
import shutil
import zipfile

POINTER = b'version https://git-lfs.github.com/spec/v1'

def manifest(source):
    files = {}
    for path in sorted(source.rglob('*')):
        if not path.is_file() or any(part.startswith('.') for part in path.relative_to(source).parts):
            continue
        with path.open('rb') as stream:
            if stream.read(len(POINTER)) == POINTER:
                raise ValueError(f'Unresolved Git LFS pointer: {path}')
            stream.seek(0)
            digest = hashlib.file_digest(stream, 'sha256').hexdigest()
        files[path.relative_to(source).as_posix()] = digest
    for required in ('SorR.dat', 'mod/system.txt', 'savegame/savegame.sor', 'xbox/xbox.cfg'):
        if required not in files:
            raise ValueError(f'Missing required game file: {required}')
    if not any(name.startswith('mod/music/') for name in files):
        raise ValueError('Missing music assets')
    return files

def stage(source, app):
    files = manifest(source)
    dest = app / 'GameData'
    if dest.exists():
        raise ValueError(f'Refusing to mix new data with an existing bundle: {dest}')
    for name in files:
        target = dest / name
        target.parent.mkdir(parents=True, exist_ok=True)
        shutil.copyfile(source / name, target)
    (app / 'GameData-manifest.json').write_text(json.dumps(files, indent=2) + '\n')
    print(f'Bundled {len(files)} verified game files in {dest}')

def verify(ipa):
    prefix = 'Payload/SorrIOSShell.app/'
    with zipfile.ZipFile(ipa) as archive:
        files = json.loads(archive.read(prefix + 'GameData-manifest.json'))
        if not files or 'SorR.dat' not in files:
            raise ValueError('Game data manifest is empty or incomplete')
        for name, expected in files.items():
            with archive.open(prefix + 'GameData/' + name) as stream:
                actual = hashlib.file_digest(stream, 'sha256').hexdigest()
            if actual != expected:
                raise ValueError(f'IPA game asset mismatch: {name}')
    print(f'IPA verified: {len(files)} bundled assets match SHA-256 manifest')

if __name__ == '__main__':
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--source', type=Path)
    parser.add_argument('--app', type=Path)
    parser.add_argument('--verify-ipa', type=Path)
    args = parser.parse_args()
    if args.verify_ipa:
        verify(args.verify_ipa)
    elif args.source and args.app:
        stage(args.source, args.app)
    else:
        parser.error('Use --source and --app, or --verify-ipa')
