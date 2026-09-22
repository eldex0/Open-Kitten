import unittest
from html.parser import HTMLParser
from core.homepage import render_homepage, ENGINES
from core.search_engines import QUERY_KEYS


class Storage:
    def load_bookmarks(self):
        return {'Favorites': [
            {'title': '<img src=x onerror=alert(1)>', 'url': 'https://example.org/?a=1&b=2'},
            {'title': 'unsafe', 'url': 'javascript:alert(1)'},
        ]}
    def load_history(self): return []


class Tags(HTMLParser):
    def __init__(self, source):
        super().__init__()
        self.tags = []
        self.feed(source)
    def handle_starttag(self, tag, attrs): self.tags.append((tag, dict(attrs)))


class HomeTests(unittest.TestCase):
    def test_search_form_for_each_engine(self):
        for engine, action in ENGINES.items():
            tags = Tags(render_homepage({'search_engine': engine}, Storage(), '')).tags
            form = next(attrs for tag, attrs in tags if tag == 'form')
            field = next(attrs for tag, attrs in tags if tag == 'input')
            self.assertEqual(form['action'], action)
            self.assertEqual(form['method'], 'get')
            self.assertEqual(field['name'], QUERY_KEYS.get(engine, 'q'))
            self.assertIn('required', field)

    def test_untrusted_bookmark_markup_and_scheme(self):
        tags = Tags(render_homepage({}, Storage(), '')).tags
        self.assertFalse(any(tag in {'img', 'script'} for tag, _ in tags))
        links = [attrs['href'] for tag, attrs in tags if tag == 'a']
        self.assertEqual(links, ['https://example.org/?a=1&b=2'])

    def test_theme_and_unknown_engine(self):
        source = render_homepage({'theme': 'light', 'search_engine': 'unknown'}, Storage(), '')
        self.assertIn('class="light"', source)
        self.assertIn('action="https://www.google.com/search"', source)


if __name__ == '__main__': unittest.main()
