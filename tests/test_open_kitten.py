import unittest
from urllib.parse import urlsplit, parse_qs
from core.app_paths import APP_NAME, VERSION
from core.i18n import UI_LANGUAGES, ROWS, KEYS, tr, set_language
from core.search_engines import ENGINES, QUERY_KEYS
from core.navigation import resolve_address
from core.settings import validated_settings
from core.homepage import render_homepage
from unittest.mock import Mock


class OpenKittenTests(unittest.TestCase):
    def tearDown(self):
        set_language('fr')

    def test_brand(self):
        self.assertEqual((APP_NAME, VERSION), ('Open Kitten', '1.0.0'))

    def test_every_engine_encodes_query(self):
        self.assertEqual(len(ENGINES), 7)
        for engine in ENGINES:
            url = urlsplit(resolve_address('chat & café', engine))
            self.assertEqual(url.scheme, 'https')
            self.assertEqual(parse_qs(url.query)[QUERY_KEYS.get(engine, 'q')], ['chat & café'])
            self.assertEqual(validated_settings({'search_engine': engine})['search_engine'], engine)

    def test_catalog_rows_are_complete_for_core_labels(self):
        for code, row in ROWS.items():
            self.assertEqual(len(row.split('|')), len(KEYS), code)
            self.assertEqual(tr('Paramètres', code), row.split('|')[6])
        self.assertEqual(len(UI_LANGUAGES), 10)

    def test_fallback_is_explicit_and_stable(self):
        set_language('en')
        self.assertEqual(tr('Paramètres'), 'Settings')
        self.assertEqual(tr('Moteur de recherche', 'ja'), '検索エンジン')
        self.assertEqual(tr('User supplied text'), 'User supplied text')
        self.assertEqual(tr('Paramètres', 'fr'), 'Paramètres')

    def test_website_languages_are_not_limited_to_ui_languages(self):
        for code in ('hi-IN', 'zh-Hant-TW', 'uk-UA', 'ar', 'sw-KE'):
            self.assertEqual(validated_settings({'language': code})['language'], code)
        for code in ('en\r\nInjected: true', '../fr', ''):
            with self.assertRaises(ValueError):
                validated_settings({'language': code})

    def test_rtl_homepage(self):
        storage = Mock()
        storage.load_bookmarks.return_value = {}
        storage.load_history.return_value = []
        html = render_homepage({'ui_language': 'ar', 'search_engine': 'Startpage'}, storage, '')
        self.assertIn('dir="rtl"', html)
        self.assertIn('name="query"', html)
        self.assertIn('Open Kitten', html)
        self.assertIn('بحث', html)


if __name__ == '__main__':
    unittest.main()
