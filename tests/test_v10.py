import ast
import json
from pathlib import Path
import tempfile
from types import SimpleNamespace
import unittest
from unittest.mock import Mock, patch

from core.settings import SettingsManager, validated_settings
from core.storage import Storage
from core.json_store import atomic_write
from core.host_filter import is_blocked

ROOT = Path(__file__).resolve().parents[1]


def method(file, class_name, method_name, **globals_):
    """Exercise real controller methods with inert views, without requiring Qt."""
    tree = ast.parse((ROOT / file).read_text(encoding='utf-8'))
    cls = next(node for node in tree.body if isinstance(node, ast.ClassDef) and node.name == class_name)
    func = next(node for node in cls.body if isinstance(node, ast.FunctionDef) and node.name == method_name)
    scope = dict(globals_)
    exec(compile(ast.fix_missing_locations(ast.Module(body=[func], type_ignores=[])), file, 'exec'), scope)
    return scope[method_name]


class SettingsTests(unittest.TestCase):
    def test_migration_preserves_old_values(self):
        settings = validated_settings({'theme': 'light', 'future_key': 42})
        self.assertTrue(settings['lazy_restore'])
        self.assertEqual(settings['theme'], 'light')
        self.assertEqual(settings['future_key'], 42)

    def test_invalid_settings(self):
        for values in ([], {'default_zoom': True}, {'default_zoom': 201},
                       {'cache_size_mb': -1}, {'theme': 'unknown'},
                       {'homepage': 'custom', 'custom_homepage': 'javascript:alert(1)'},
                       {'proxy_enabled': True, 'proxy_host': 'example.com', 'proxy_port': '70000'}):
            with self.subTest(values=values), self.assertRaises(ValueError):
                validated_settings(values)

    def test_save_failure_does_not_change_memory(self):
        with tempfile.TemporaryDirectory() as directory:
            manager = SettingsManager(directory)
            with patch('core.settings.atomic_write', side_effect=OSError('disk full')):
                with self.assertRaises(OSError):
                    manager.update({'theme': 'light'})
            self.assertEqual(manager.get('theme'), 'dark')

    def test_roundtrip_and_corrupt_file(self):
        with tempfile.TemporaryDirectory() as directory:
            manager = SettingsManager(directory)
            manager.update({'theme': 'light', 'cache_size_mb': 256})
            self.assertEqual(SettingsManager(directory).get('cache_size_mb'), 256)
            Path(manager.path).write_text('{broken', encoding='utf-8')
            self.assertEqual(SettingsManager(directory).get('theme'), 'dark')


class PersistenceTests(unittest.TestCase):
    def test_failed_replace_preserves_file(self):
        with tempfile.TemporaryDirectory() as directory:
            path = Path(directory) / 'settings.json'
            atomic_write(path, {'old': True})
            with patch('core.json_store.os.replace', side_effect=OSError('locked')):
                with self.assertRaises(OSError):
                    atomic_write(path, {'new': True})
            self.assertEqual(json.loads(path.read_text()), {'old': True})
            self.assertEqual(list(Path(directory).glob('*.tmp')), [])

    def test_cached_reads_are_independent(self):
        with tempfile.TemporaryDirectory() as directory:
            storage = Storage(directory)
            storage.add_bookmark('Example', 'https://example.com')
            with patch('builtins.open', side_effect=AssertionError('unexpected disk read')):
                self.assertTrue(storage.is_bookmarked('https://example.com'))
                data = storage.load_bookmarks()
                data['Unsorted'].clear()
                self.assertTrue(storage.is_bookmarked('https://example.com'))

    def test_unchanged_session_is_not_rewritten(self):
        with tempfile.TemporaryDirectory() as directory:
            storage = Storage(directory)
            storage.save_session(['https://example.com'])
            with patch('core.storage.atomic_write') as write:
                storage.save_session(['https://example.com'])
                write.assert_not_called()

    def test_session_ignores_invalid_entries(self):
        with tempfile.TemporaryDirectory() as directory:
            storage = Storage(directory)
            storage.save_session([None, {}, 'data:text/html,hello', 'https://example.com'])
            self.assertEqual(storage.load_session(), ['https://example.com'])
            storage.save_session('invalid')
            self.assertEqual(storage.load_session(), [])


class ControllerTests(unittest.TestCase):
    def test_closed_tabs_use_lifo_not_history(self):
        reopen = method('ui/browser.py', 'MiniBrowser', 'restore_closed_tab')
        browser = SimpleNamespace(closed_tabs=['https://a.test', 'https://b.test'], add_tab=Mock())
        reopen(browser)
        browser.add_tab.assert_called_once_with('https://b.test', True)
        self.assertEqual(browser.closed_tabs, ['https://a.test'])

    def test_pending_tab_loads_once(self):
        activate = method('ui/tabs.py', 'BrowserTab', 'activate', QUrl=lambda value: value)
        tab = SimpleNamespace(pending_url='https://example.com', browser=Mock())
        activate(tab)
        activate(tab)
        tab.browser.setUrl.assert_called_once_with('https://example.com')

    def test_pending_tabs_survive_session_save(self):
        save = method('ui/browser.py', 'MiniBrowser', 'save_session')
        browser = SimpleNamespace(settings={'autosave_session': True}, storage=Mock(),
            browser_tabs=[SimpleNamespace(session_url=lambda: 'https://pending.test'),
                          SimpleNamespace(session_url=lambda: 'data:text/html,home')])
        save(browser)
        browser.storage.save_session.assert_called_once_with(['https://pending.test'])

    def test_close_save_independent_from_autosave(self):
        save = method('ui/browser.py', 'MiniBrowser', 'save_session')
        browser = SimpleNamespace(settings={'autosave_session': False}, storage=Mock(), browser_tabs=[])
        save(browser)
        browser.storage.save_session.assert_not_called()
        save(browser, force=True)
        browser.storage.save_session.assert_called_once_with([])

    def test_restore_defers_only_background_tabs(self):
        restore = method('ui/browser.py', 'MiniBrowser', 'restore_or_create_session')
        browser = SimpleNamespace(settings={'restore_session': True, 'lazy_restore': True},
                                  storage=Mock(), add_tab=Mock())
        browser.storage.load_session.return_value = ['https://a.test', 'https://b.test']
        restore(browser)
        self.assertEqual(browser.add_tab.call_args_list[0].kwargs, {'switch': True, 'deferred': False})
        self.assertEqual(browser.add_tab.call_args_list[1].kwargs, {'switch': False, 'deferred': True})

    def test_domain_boundaries(self):
        blocked = {'tracker.test'}
        for host in ('tracker.test', 'a.tracker.test', 'A.TRACKER.TEST.'):
            self.assertTrue(is_blocked(host, blocked))
        for host in ('nottracker.test', 'tracker.test.good.test', '', 'example.com'):
            self.assertFalse(is_blocked(host, blocked))


if __name__ == '__main__':
    unittest.main()
