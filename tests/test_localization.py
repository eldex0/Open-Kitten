import ast
import json
from pathlib import Path
from string import Formatter
import tempfile
import unittest
from unittest.mock import Mock

from core.i18n import (EN, CATALOGS, UI_LANGUAGES, resolve_language, set_language,
                       set_system_languages, tr, website_language)
from core.settings import SettingsManager, validated_settings
from core.homepage import render_homepage

ROOT = Path(__file__).resolve().parents[1]


class LocalizationTests(unittest.TestCase):
    def tearDown(self):
        set_system_languages(['en'])
        set_language('fr')

    def test_all_registered_messages_have_translations(self):
        self.assertGreater(len(EN), 220)
        for language in UI_LANGUAGES:
            if language == 'fr':
                continue
            self.assertEqual(set(EN), set(CATALOGS[language]), language)
            for source, target in CATALOGS[language].items():
                self.assertTrue(target.strip(), (language, source))
                fields = lambda text: {name for _, name, _, _ in Formatter().parse(text) if name}
                self.assertEqual(fields(source), fields(target), (language, source))
                self.assertEqual(source.count('\n'), target.count('\n'), (language, source))

    def test_every_literal_translation_call_has_a_catalog_entry(self):
        for folder in ('ui', 'core'):
            for path in (ROOT / folder).glob('*.py'):
                tree = ast.parse(path.read_text(encoding='utf-8-sig'))
                for node in ast.walk(tree):
                    if isinstance(node, ast.Call) and isinstance(node.func, ast.Name) and node.func.id == 'tr':
                        if node.args and isinstance(node.args[0], ast.Constant):
                            self.assertIn(node.args[0].value, EN, (path.name, node.lineno))

    def test_region_and_script_detection(self):
        for tags, expected in [(['fr-BE'], 'fr'), (['pt_BR'], 'pt'),
                               (['zh-Hant-TW'], 'zh'), (['ar-SA'], 'ar'),
                               (['uk-UA', 'de-DE'], 'de'), (['unknown'], 'en'), ([], 'en')]:
            self.assertEqual(resolve_language('auto', tags), expected)

    def test_manual_choice_overrides_system(self):
        self.assertEqual(resolve_language('ja', ['fr-BE']), 'ja')
        set_system_languages(['de-DE'])
        set_language('auto')
        self.assertEqual(tr('Paramètres'), 'Einstellungen')
        set_language('fr')
        self.assertEqual(tr('Paramètres'), 'Paramètres')

    def test_new_install_defaults_to_auto_and_saved_choice_survives(self):
        with tempfile.TemporaryDirectory() as directory:
            manager = SettingsManager(directory)
            self.assertEqual(manager.get('ui_language'), 'auto')
            self.assertEqual(manager.get('language'), 'auto')
            manager.set('ui_language', 'es')
            self.assertEqual(SettingsManager(directory).get('ui_language'), 'es')
            manager.set('ui_language', 'auto')
            self.assertEqual(SettingsManager(directory).get('ui_language'), 'auto')

    def test_migration_preserves_explicit_previous_language(self):
        self.assertEqual(validated_settings({'ui_language': 'fr'})['ui_language'], 'fr')
        self.assertEqual(validated_settings({})['ui_language'], 'auto')

    def test_website_language_preserves_region_and_manual_selection(self):
        set_system_languages(['pt-BR'])
        self.assertEqual(website_language('auto'), 'pt-BR')
        self.assertEqual(website_language('nl-BE'), 'nl-BE')
        set_system_languages(['en\r\nInjected: true'])
        self.assertEqual(website_language('auto'), 'en')

    def test_auto_rtl_and_translated_homepage(self):
        set_system_languages(['ar-SA'])
        storage = Mock()
        storage.load_bookmarks.return_value = {}
        storage.load_history.return_value = []
        html = render_homepage({'ui_language': 'auto'}, storage, '')
        self.assertIn('lang="ar" dir="rtl"', html)
        self.assertNotIn('Vos favoris', html)
        self.assertIn(tr('Vos favoris apparaîtront ici.', 'ar'), html)


if __name__ == '__main__':
    unittest.main()
