"""Apply profile defaults once so existing and future pages inherit them."""
from PyQt6.QtWebEngineCore import QWebEngineSettings
from core.i18n import website_language


def apply_web_settings(profile, settings):
    profile.setHttpAcceptLanguage(website_language(settings.get('language', 'auto')) + ',en;q=0.7')
    profile.setHttpCacheMaximumSize(settings.get('cache_size_mb', 0) * 1024 * 1024)
    attributes = QWebEngineSettings.WebAttribute
    for attribute, enabled in (
        (attributes.DnsPrefetchEnabled, settings.get('dns_prefetch', True)),
        (attributes.JavascriptCanOpenWindows, not settings.get('block_popups', True)),
        (attributes.PlaybackRequiresUserGesture, not settings.get('autoplay', True)),
    ):
        profile.settings().setAttribute(attribute, enabled)
