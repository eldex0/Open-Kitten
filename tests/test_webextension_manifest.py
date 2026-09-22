import json
import tempfile
import unittest
import zipfile
from pathlib import Path
from core.webextension_manifest import inspect_webextension


class NativeManifestTests(unittest.TestCase):
    def setUp(self):
        self.temp = tempfile.TemporaryDirectory()
        self.addCleanup(self.temp.cleanup)
        self.root = Path(self.temp.name)
        self.manifest = {'manifest_version': 3, 'name': 'Test', 'version': '1.0',
                         'permissions': ['storage'], 'host_permissions': ['https://example.org/*']}

    def write(self):
        (self.root / 'manifest.json').write_text(json.dumps(self.manifest), encoding='utf-8')

    def test_folder_and_zip(self):
        self.write()
        expected = ('Test', ['storage', 'https://example.org/*'])
        self.assertEqual(inspect_webextension(self.root), expected)
        archive = self.root / 'extension.zip'
        with zipfile.ZipFile(archive, 'w') as z:
            z.writestr('manifest.json', json.dumps(self.manifest))
        self.assertEqual(inspect_webextension(archive), expected)

    def test_mv2_and_firefox_rejected(self):
        self.manifest['manifest_version'] = 2
        self.write()
        with self.assertRaises(ValueError): inspect_webextension(self.root)
        self.manifest['manifest_version'] = 3
        self.manifest['browser_specific_settings'] = {'gecko': {'id': 'test'}}
        self.write()
        with self.assertRaises(ValueError): inspect_webextension(self.root)

    def test_path_escape(self):
        archive = self.root / 'extension.zip'
        for name in ('../escape.js', 'C:/escape.js', '/escape.js', '..\\escape.js'):
            with zipfile.ZipFile(archive, 'w') as z:
                z.writestr('manifest.json', json.dumps(self.manifest))
                z.writestr(name, '')
            with self.assertRaises(ValueError): inspect_webextension(archive)

    def test_invalid_permissions(self):
        self.manifest['permissions'] = 'storage'
        self.write()
        with self.assertRaises(ValueError): inspect_webextension(self.root)

    def test_xpi_not_silently_converted(self):
        with self.assertRaises(ValueError): inspect_webextension(self.root / 'test.xpi')


if __name__ == '__main__': unittest.main()
