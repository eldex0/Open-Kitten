import os
import sys

# ============================================================
# IMPORTANT :
# La configuration du proxy doit être faite AVANT l'import
# de QWebEngine / MiniBrowser.
# ============================================================

from core.settings import SettingsManager
from core.proxy import configure_proxy_environment
from core.app_paths import RESOURCE_DIR, VERSION, data_directory
from core.i18n import set_language, set_system_languages, resolve_language


BASE_DIR = str(RESOURCE_DIR)
DATA_DIR = str(data_directory())
ASSETS_DIR = os.path.join(BASE_DIR, "assets")

os.makedirs(DATA_DIR, exist_ok=True)


# Charger les paramètres avant QtWebEngine
settings = SettingsManager(DATA_DIR)
configure_proxy_environment(settings)


from PyQt6.QtGui import QIcon
from PyQt6.QtWidgets import QApplication
from PyQt6.QtCore import Qt, QLocale

from ui.browser import MiniBrowser
from ui.splash import StartupSplash
from ui.localization import install_qt_translations


def main():
    app = QApplication(sys.argv)
    set_system_languages(QLocale.system().uiLanguages())
    language = resolve_language(settings.get('ui_language', 'auto'))
    set_language(language)
    install_qt_translations(app, language)
    QLocale.setDefault(QLocale(language))
    app.setLayoutDirection(Qt.LayoutDirection.RightToLeft if language == 'ar'
                           else Qt.LayoutDirection.LeftToRight)

    app.setApplicationName("Open Kitten")
    app.setApplicationDisplayName("Open Kitten " + VERSION)
    app.setApplicationVersion(VERSION)
    app.setOrganizationName("Open Kitten")

    icon_path = os.path.join(ASSETS_DIR, "icon.svg")

    if os.path.exists(icon_path):
        app.setWindowIcon(QIcon(icon_path))

    splash = StartupSplash() if settings.get('show_splash', True) else None
    if splash:
        splash.show()
        app.processEvents()

    browser = MiniBrowser(settings)

    def show_browser():
        splash.close()
        browser.show()

    if splash:
        splash.finished.connect(show_browser)
        splash.start()
    else:
        browser.show()

    sys.exit(app.exec())


if __name__ == "__main__":
    main()
