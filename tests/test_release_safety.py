import ast
import os
import tempfile
import unittest
from pathlib import Path
from unittest.mock import patch

from core.profiles import ProfileManager
from core.app_paths import data_directory

ROOT = Path(__file__).resolve().parents[1]


class ReleaseSafetyTests(unittest.TestCase):
    def test_profile_names_stay_below_root(self):
        with tempfile.TemporaryDirectory() as directory:
            manager = ProfileManager(directory)
            for name in ('..', '.', '../escape', '..\\escape', 'CON', 'LPT1', 'test. ', ''):
                path = Path(manager.get_profile_dir(name)).resolve()
                self.assertEqual(path.parent, Path(manager.profiles_dir).resolve())
                self.assertNotIn(path.name, ('', '.', '..', 'CON', 'LPT1'))

    def test_default_profile_cannot_be_deleted(self):
        with tempfile.TemporaryDirectory() as directory:
            manager = ProfileManager(directory)
            self.assertFalse(manager.delete_profile('..'))
            self.assertFalse(manager.delete_profile('Default'))

    def test_packaged_data_is_user_local(self):
        with patch('sys.frozen', True, create=True), patch.dict(os.environ, {'LOCALAPPDATA': '/example/user'}):
            self.assertEqual(data_directory(), Path('/example/user/MiniBrowser/data'))

    def test_source_data_location_preserved(self):
        with patch('sys.frozen', False, create=True):
            self.assertEqual(data_directory(), ROOT / 'data')

    def test_all_sources_parse(self):
        for path in list((ROOT / 'core').glob('*.py')) + list((ROOT / 'ui').glob('*.py')) + [ROOT / 'main.py']:
            ast.parse(path.read_text(encoding='utf-8-sig'), filename=str(path))

    def test_package_resource_allowlist(self):
        # Execute the spec with inert PyInstaller constructors to inspect its inputs.
        recorded = {}
        def analysis(*args, **kwargs):
            recorded.update(kwargs)
            return type('AnalysisResult', (), dict(pure=[], scripts=[], binaries=[], datas=kwargs['datas']))()
        scope = dict(SPECPATH=str(ROOT / 'packaging'), Analysis=analysis,
                     PYZ=lambda *a, **kw: None, EXE=lambda *a, **kw: None,
                     COLLECT=lambda *a, **kw: None)
        spec = ROOT / 'packaging/MiniBrowser.spec'
        exec(compile(spec.read_text(), str(spec), 'exec'), scope)
        self.assertTrue(recorded['datas'])
        for source, dest in recorded['datas']:
            self.assertTrue(Path(source).is_file())
            self.assertNotEqual(Path(source).name, 'state.json')
            self.assertNotIn('data', Path(source).relative_to(ROOT).parts)


if __name__ == '__main__':
    unittest.main()
