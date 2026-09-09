import importlib.util
from pathlib import Path
import tempfile
import unittest
import zipfile

spec = importlib.util.spec_from_file_location('bundle', Path(__file__).resolve().parents[1] / 'tools/bundle_game_data.py')
bundle = importlib.util.module_from_spec(spec)
spec.loader.exec_module(bundle)

class BundleTests(unittest.TestCase):
    def setUp(self):
        self.temp = tempfile.TemporaryDirectory()
        self.addCleanup(self.temp.cleanup)
        self.root = Path(self.temp.name)
        self.source = self.root / 'source'
        for name in ('SorR.dat', 'mod/system.txt', 'mod/music/1.ogg', 'savegame/savegame.sor', 'xbox/xbox.cfg'):
            path = self.source / name
            path.parent.mkdir(parents=True, exist_ok=True)
            path.write_bytes(b'fixture-content-' + name.encode())
        self.app = self.root / 'Payload/SorrIOSShell.app'
        self.app.mkdir(parents=True)

    def zip_app(self):
        ipa = self.root / 'game.ipa'
        with zipfile.ZipFile(ipa, 'w') as z:
            for path in self.app.rglob('*'):
                if path.is_file(): z.write(path, path.relative_to(self.root))
        return ipa

    def test_roundtrip_and_corruption(self):
        bundle.stage(self.source, self.app)
        bundle.verify(self.zip_app())
        (self.app / 'GameData/mod/music/1.ogg').write_bytes(b'corrupted')
        with self.assertRaisesRegex(ValueError, 'mismatch'): bundle.verify(self.zip_app())

    def test_lfs_pointer_rejected(self):
        (self.source / 'SorR.dat').write_bytes(bundle.POINTER + b'\noid sha256:missing\nsize 99\n')
        with self.assertRaisesRegex(ValueError, 'Unresolved'): bundle.stage(self.source, self.app)

    def test_missing_music_rejected(self):
        (self.source / 'mod/music/1.ogg').unlink()
        with self.assertRaisesRegex(ValueError, 'music'): bundle.stage(self.source, self.app)

    def test_existing_bundle_not_mixed(self):
        bundle.stage(self.source, self.app)
        with self.assertRaisesRegex(ValueError, 'existing bundle'): bundle.stage(self.source, self.app)

if __name__ == '__main__': unittest.main()
