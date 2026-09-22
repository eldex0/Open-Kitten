from core.i18n import tr
from core.navigation import resolve_address
from core.web_settings import apply_web_settings

from PyQt6.QtCore import QUrl, Qt, pyqtSignal
from PyQt6.QtGui import QColor
from ui.icons import icon
from PyQt6.QtWidgets import (
    QLineEdit,
    QMainWindow,
    QToolBar,
)
from PyQt6.QtWebEngineCore import (
    QWebEngineProfile,
    QWebEnginePage,
)
from PyQt6.QtWebEngineWidgets import (
    QWebEngineView,
)


class PrivateBrowser(QMainWindow):

    closed = pyqtSignal(object)

    def __init__(self, parent=None):

        super().__init__(parent)

        self.setWindowTitle(
            "Open Kitten — " + tr('Navigation privée')
        )

        self.resize(
            1200,
            800
        )

        self.setAttribute(
            Qt.WidgetAttribute.WA_DeleteOnClose
        )

        # Sans nom de stockage, QtWebEngine crée un profil temporaire qui
        # n'écrit aucune donnée de navigation sur le disque.
        self.profile = QWebEngineProfile(
            self
        )

        self.profile.setPersistentCookiesPolicy(
            QWebEngineProfile.PersistentCookiesPolicy.NoPersistentCookies
        )

        self.profile.setHttpCacheType(
            QWebEngineProfile.HttpCacheType.MemoryHttpCache
        )

        self.settings = parent.settings if parent is not None else {}
        if parent is not None:
            apply_web_settings(self.profile, self.settings)

        self.browser = QWebEngineView()
        self.browser.setStyleSheet("background-color: #101116;")

        self.page = QWebEnginePage(
            self.profile,
            self.browser
        )
        self.page.setBackgroundColor(QColor("#101116"))

        self.browser.setPage(
            self.page
        )

        self.browser.setZoomFactor(self.settings.get('default_zoom', 100) / 100)

        self.setCentralWidget(
            self.browser
        )

        self.setup_navigation()

        self.browser.urlChanged.connect(
            self.update_address_bar
        )

        self.browser.setUrl(
            QUrl(
                "https://www.google.com/"
            )
        )

    def setup_navigation(self):

        toolbar = QToolBar(
            tr("Navigation privée"),
            self
        )

        toolbar.setMovable(False)

        self.addToolBar(toolbar)

        back_action = toolbar.addAction(icon('back'), tr('Précédent'))
        forward_action = toolbar.addAction(icon('forward'), tr('Suivant'))
        reload_action = toolbar.addAction(icon('reload'), tr('Actualiser'))

        self.address_bar = QLineEdit()

        self.address_bar.setPlaceholderText(
            tr("Rechercher ou saisir une adresse...")
        )

        toolbar.addWidget(self.address_bar)

        back_action.triggered.connect(self.browser.back)
        forward_action.triggered.connect(self.browser.forward)
        reload_action.triggered.connect(self.browser.reload)
        self.address_bar.returnPressed.connect(self.navigate)

    def navigate(self):
        url = resolve_address(self.address_bar.text(), self.settings.get('search_engine', 'Google'))
        if url:
            self.browser.setUrl(QUrl(url))

    def update_address_bar(self, url):

        text = url.toString()

        if text.startswith("data:text/html"):
            self.address_bar.clear()
        else:
            self.address_bar.setText(text)

    def closeEvent(self, event):

        self.browser.stop()
        from PyQt6 import sip
        sip.delete(self.page)
        self.closed.emit(self)
        event.accept()
