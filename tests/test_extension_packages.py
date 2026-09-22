import json
import tempfile
import unittest
from pathlib import Path
from core.extension_packages import read_package, install_package


class PackageTests(unittest.TestCase):
    def setUp(self):
        self.temp = tempfile.TemporaryDirectory()
        self.addCleanup(self.temp.cleanup)
        self.source = Path(self.temp.name) / 'source'
        self.source.mkdir()
        self.dest = Path(self.temp.name) / 'installed'
        self.manifest = {'name': 'Example', 'version': '1', 'script': 'script.js',
                         'enabled_by_default': True}
        (self.source / 'script.js').write_text('document.title = "test";', encoding='utf-8')
        self.write()

    def write(self):
        (self.source / 'manifest.json').write_text(json.dumps(self.manifest), encoding='utf-8')

    def test_install_disabled_and_independent_of_source(self):
        (self.source / 'secret.txt').write_text('not an extension file')
        identity = install_package(self.source, self.dest)
        self.assertEqual(set(p.name for p in (self.dest / identity).iterdir()), {'manifest.json', 'script.js'})
        (self.source / 'script.js').write_text('changed')
        manifest, script = read_package(self.dest / identity)
        self.assertFalse(manifest['enabled_by_default'])
        self.assertIn('document.title', script)

    def test_duplicate_does_not_overwrite(self):
        identity = install_package(self.source, self.dest)
        with self.assertRaises(ValueError): install_package(self.source, self.dest)
        self.assertTrue((self.dest / identity / 'script.js').exists())

    def test_unsafe_script_paths(self):
        for script in ('../escape.js', '/absolute.js', 'C:\\escape.js', 'sub/file.js', 'file.js:secret'):
            self.manifest['script'] = script
            self.write()
            with self.assertRaises(ValueError): install_package(self.source, self.dest)

    def test_chrome_manifest_rejected(self):
        self.manifest['manifest_version'] = 3
        self.write()
        with self.assertRaises(ValueError): read_package(self.source)

    def test_invalid_domain_and_empty_package(self):
        self.manifest.pop('script')
        for hosts in ([], ['https://example.com'], ['*.example.com'], [123]):
            self.manifest['blocked_hosts'] = hosts
            self.write()
            with self.assertRaises(ValueError): read_package(self.source)

    def test_domain_only_package(self):
        self.manifest.pop('script')
        self.manifest['blocked_hosts'] = ['ads.example.org']
        self.write()
        identity = install_package(self.source, self.dest)
        data, script = read_package(self.dest / identity)
        self.assertEqual(data['blocked_hosts'], ['ads.example.org'])
        self.assertIsNone(script)

    def test_shipped_manifests(self):
        root = Path(__file__).resolve().parents[1]
        for manifest in (root / 'extensions').glob('*/manifest.json'):
            read_package(manifest.parent)


if __name__ == '__main__': unittest.main()
