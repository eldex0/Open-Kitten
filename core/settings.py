import json
import os
import re
from urllib.parse import urlsplit
from core.json_store import atomic_write
from core.extension_options import parse_sites
from core.search_engines import ENGINES
from core.i18n import UI_LANGUAGES, tr


DEFAULT_SETTINGS = {
    "ui_language": "auto",
    "language": "auto",
    "search_engine": "Google",

    "theme": "dark",

    "homepage": "newtab",
    "custom_homepage": "",

    "show_bookmark_bar": True,
    "block_popups": True,

    "download_directory": "",

    "restore_session": True,

    "proxy_enabled": False,
    "proxy_type": "HTTP",
    "proxy_host": "",
    "proxy_port": "",
    "proxy_username": "",
    "proxy_password": "",

    "default_profile": "Default",

    "autosave_session": True,

    "crash_recovery": True,
    "lazy_restore": True,
    "dns_prefetch": True,
    "cache_size_mb": 0,
    "default_zoom": 100,
    "remember_history": True,
    "autoplay": True,
    "show_splash": True,
    "accent_color": "blue",
    "ui_font_size": 13,
    "show_home_button": True,
    "show_status_bar": True,
    "home_show_logo": True,
    "home_shortcut_count": 8,
    "home_history_suggestions": True,
    "ad_cosmetic": True,
    "privacy_links": True,
    "dark_respect_native": True,
    "dark_palette": "neutral",
    "ad_blocker_exceptions": "",
    "privacy_guard_exceptions": "",
    "dark_mode_exceptions": "",
}


def validated_settings(values):
    """Validate known values without discarding future or legacy keys."""
    if not isinstance(values, dict):
        raise ValueError(tr("Le fichier de paramètres doit contenir un objet."))
    result = DEFAULT_SETTINGS.copy()
    result.update(values)
    for key, default in DEFAULT_SETTINGS.items():
        if type(result[key]) is not type(default):
            raise ValueError(tr("Valeur incorrecte : ") + key)
    choices = {'theme': ('dark', 'light'), 'homepage': ('newtab', 'custom'),
               'accent_color': ('blue', 'green', 'purple', 'orange'),
               'dark_palette': ('neutral', 'warm'),
               'search_engine': tuple(ENGINES),
               'ui_language': ('auto', *UI_LANGUAGES),
               'proxy_type': ('HTTP', 'HTTPS', 'SOCKS5'),
               }
    if result['language'] != 'auto' and not re.fullmatch(r'[a-zA-Z]{2,3}(?:-[a-zA-Z0-9]{2,8})*', result['language']):
        raise ValueError(tr('Code de langue invalide.'))
    for key, allowed in choices.items():
        if result[key] not in allowed:
            raise ValueError(tr("Choix incorrect : ") + key)
    for key, low, high in [('cache_size_mb', 0, 2048), ('default_zoom', 50, 200),
                           ('ui_font_size', 11, 18), ('home_shortcut_count', 0, 16)]:
        if not low <= result[key] <= high:
            raise ValueError(tr('{key} doit être compris entre {low} et {high}.').format(key=key, low=low, high=high))
    for name in ('ad_blocker', 'privacy_guard', 'dark_mode'):
        parse_sites(result[name + '_exceptions'])
    if result['homepage'] == 'custom':
        url = urlsplit(result['custom_homepage'])
        if url.scheme not in ('http', 'https') or not url.hostname:
            raise ValueError(tr("La page d’accueil doit être une adresse http:// ou https:// valide."))
    if result['proxy_enabled']:
        host = result['proxy_host']
        if not host or any(c.isspace() or c in '/@' for c in host):
            raise ValueError(tr("Adresse du proxy incorrecte (sans protocole ni chemin)."))
        port = result['proxy_port']
        if not port.isdecimal() or not 1 <= int(port) <= 65535:
            raise ValueError(tr("Le port du proxy doit être compris entre 1 et 65535."))
    return result


class SettingsManager:

    def __init__(self, data_dir):
        self.data_dir = data_dir
        self.path = os.path.join(data_dir, "settings.json")

        os.makedirs(self.data_dir, exist_ok=True)

        self.settings = self.load()

    def load(self):
        if not os.path.exists(self.path):
            return DEFAULT_SETTINGS.copy()

        try:
            with open(self.path, "r", encoding="utf-8") as file:
                data = json.load(file)

            return validated_settings(data)

        except Exception:
            return DEFAULT_SETTINGS.copy()

    def save(self):
        atomic_write(self.path, self.settings)

    def get(self, key, default=None):
        return self.settings.get(key, default)

    def set(self, key, value):
        self.update({key: value})

    def update(self, values):
        candidate = validated_settings({**self.settings, **values})
        atomic_write(self.path, candidate)
        self.settings = candidate

    def reset(self):
        self.update(DEFAULT_SETTINGS)
