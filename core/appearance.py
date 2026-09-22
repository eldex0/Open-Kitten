"""Shared, allowlisted accent colors for widgets and the start page."""
ACCENTS = {
    'blue': ('#315bdb', '#92aaff'),
    'green': ('#14734d', '#70d6ab'),
    'purple': ('#7540bf', '#c8a3fa'),
    'orange': ('#a34812', '#ffb47d'),
}


def accent_color(settings):
    pair = ACCENTS.get(settings.get('accent_color', 'blue'), ACCENTS['blue'])
    return pair[1 if settings.get('theme', 'dark') == 'dark' else 0]
