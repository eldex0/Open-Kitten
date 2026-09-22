import json
from pathlib import Path
import unittest
from core.settings import validated_settings
from core.extension_options import parse_sites, should_block, configure_script
from core.homepage import render_homepage
from ui.theme import stylesheet
from unittest.mock import Mock


class ExtensionV2Tests(unittest.TestCase):
    def test_site_validation(self):
        self.assertEqual(parse_sites('Example.com, sub.test; example.com'), {'example.com', 'sub.test'})
        for value in ('https://example.com', '*.test', 'test/path', 'bad..test', '-bad.test'):
            with self.assertRaises(ValueError):
                parse_sites(value)

    def test_exceptions_are_per_extension_and_on_domain_boundaries(self):
        groups = [({'ad.test'}, {'allowed.test'}), ({'tracking.test'}, set())]
        self.assertFalse(should_block('cdn.ad.test', 'www.allowed.test', groups))
        self.assertTrue(should_block('ad.test', 'notallowed.test', groups))
        self.assertTrue(should_block('tracking.test', 'allowed.test', groups))
        self.assertFalse(should_block('notad.test', 'other.test', groups))

    def test_all_builtins_v2_and_preserve_disabled_default(self):
        root = Path(__file__).resolve().parents[1] / 'extensions'
        for name in ('ad_blocker', 'privacy_guard', 'dark_mode'):
            data = json.loads((root / name / 'manifest.json').read_text())
            self.assertEqual(data['version'], '2.0.0')
            self.assertFalse(data['enabled_by_default'])
            self.assertTrue((root / name / data['script']).is_file())

    def test_script_wrapper_is_limited_to_builtins(self):
        self.assertEqual(configure_script('user-example', 'source', {}), 'source')
        source = configure_script('ad_blocker', 'source', {'ad_blocker_exceptions': 'example.com'})
        self.assertIn('"example.com"', source)
        self.assertIn('location.protocol', source)
        self.assertIn('const minibrowserOptions', source)

    def test_invalid_customization_rejected(self):
        for settings in ({'accent_color': 'red'}, {'ui_font_size': 100},
                         {'home_shortcut_count': 200}, {'dark_palette': 'bad'},
                         {'privacy_guard_exceptions': 'https://example.com'}):
            with self.assertRaises(ValueError):
                validated_settings(settings)

    def test_appearance_values_reach_styles(self):
        css = stylesheet('light', {'theme': 'light', 'accent_color': 'green', 'ui_font_size': 17})
        self.assertIn('#14734d', css)
        self.assertIn('font-size: 17px', css)

    def test_homepage_customization(self):
        storage = Mock()
        storage.load_bookmarks.return_value = {}
        storage.load_history.return_value = [{'url': 'https://example.com'}]
        html = render_homepage({'home_history_suggestions': False, 'home_show_logo': False,
                                'home_shortcut_count': 0, 'accent_color': 'green'}, storage, '<svg/>')
        storage.load_history.assert_not_called()
        self.assertNotIn('<svg/>', html)
        self.assertNotIn('class="card"', html)
        self.assertIn('#70d6ab', html)


if __name__ == '__main__':
    unittest.main()
