from pathlib import Path
import tempfile
from types import SimpleNamespace
import unittest
from unittest.mock import Mock
import xml.etree.ElementTree as ET

from core.navigation import resolve_address
from core.storage import Storage
from core.homepage import render_homepage
from ui.theme import stylesheet
from test_v10 import method


class NavigationTests(unittest.TestCase):
    def test_addresses(self):
        for text, expected in [
            ('example.org/path', 'https://example.org/path'),
            ('localhost:8000', 'http://localhost:8000'),
            ('127.0.0.1:3000', 'http://127.0.0.1:3000'),
            ('[::1]:8000', 'http://[::1]:8000'),
            ('https://example.org/a?b=1', 'https://example.org/a?b=1'),
            ('   ', ''),
        ]:
            self.assertEqual(resolve_address(text), expected)

    def test_search_encoding_and_engine(self):
        self.assertEqual(resolve_address('chat & café', 'DuckDuckGo'),
                         'https://duckduckgo.com/?q=chat%20%26%20caf%C3%A9')
        self.assertTrue(resolve_address('hello', 'Bing').startswith('https://www.bing.com/search?q='))

    def test_malformed_input_does_not_crash(self):
        for text in ('https://[broken', 'localhost:abc', 'example.org:99999',
                     'javascript:alert(1)', 'data:text/html,hello', 'user:password@example.org'):
            self.assertTrue(resolve_address(text).startswith('https://www.google.com/search?q='))

    def test_pinned_tab_is_not_closed(self):
        close = method('ui/browser.py', 'MiniBrowser', 'close_tab')
        browser = SimpleNamespace(tabs=Mock(), browser_tabs=[SimpleNamespace(pinned=True)])
        browser.tabs.count.return_value = 1
        close(browser, 0)
        browser.tabs.removeTab.assert_not_called()

    def test_failed_load_resets_loading_controls(self):
        finished = method('ui/tabs.py', 'BrowserTab', 'load_finished')
        window = Mock()
        tab = SimpleNamespace(loading=True, progress=50, browser_window=window)
        window.current_tab.return_value = tab
        finished(tab, False)
        self.assertFalse(tab.loading)
        window.update_navigation_state.assert_called_once()
        window.storage.add_history.assert_not_called()

    def test_background_progress_does_not_change_active_ui(self):
        progress = method('ui/tabs.py', 'BrowserTab', 'load_progress')
        window = Mock()
        window.current_tab.return_value = object()
        tab = SimpleNamespace(browser_window=window)
        progress(tab, 52)
        self.assertEqual(tab.progress, 52)
        window.update_navigation_state.assert_not_called()


class PolishTests(unittest.TestCase):
    def test_corrupt_bookmark_and_history_entries(self):
        with tempfile.TemporaryDirectory() as directory:
            storage = Storage(directory)
            storage.save_bookmarks({'bad': None, 'good': [None, 9, {'url': []},
                                                        {'title': None, 'url': 'https://example.org'}]})
            self.assertEqual(storage.load_bookmarks()['bad'], [])
            self.assertTrue(storage.is_bookmarked('https://example.org'))
            storage.save_history([None, 2, {'url': 'https://example.org'}])
            self.assertEqual(len(storage.load_history()), 1)

    def test_homepage_deduplicates_and_skips_invalid_urls(self):
        storage = Mock()
        storage.load_bookmarks.return_value = {'folder': [
            {'url': 'https://[bad'}, {'url': 'https://example.org'},
            {'url': 'https://example.org'}, {'url': 'https:///missing-host'},
        ]}
        html = render_homepage({}, storage, '')
        self.assertEqual(html.count('class="card"'), 1)

    def test_both_palettes_render(self):
        for theme in ('light', 'dark', 'invalid'):
            css = stylesheet(theme)
            self.assertIn('#pageLoadProgress', css)
            self.assertNotIn('%(', css)

    def test_logo_is_flat_vector(self):
        logo = Path(__file__).resolve().parents[1] / 'assets/icon.svg'
        root = ET.parse(logo).getroot()
        tags = {element.tag.rsplit('}', 1)[-1] for element in root.iter()}
        self.assertNotIn('image', tags)
        self.assertNotIn('linearGradient', tags)
        self.assertIn('path', tags)


if __name__ == '__main__':
    unittest.main()
