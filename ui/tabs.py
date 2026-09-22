from core.i18n import tr
from PyQt6.QtCore import QUrl, pyqtSignal
from PyQt6.QtGui import QColor
from PyQt6.QtWebEngineCore import QWebEnginePage
from PyQt6.QtWebEngineWidgets import QWebEngineView
from PyQt6.QtWidgets import QTabBar


class BrowserPage(QWebEnginePage):

    def __init__(self, browser_window, view):
        super().__init__(browser_window.profile, view)
        self.setBackgroundColor(QColor("#101116"))

        self.browser_window = browser_window
        self.view = view

    def createWindow(self, window_type):

        if self.browser_window.settings.get(
            "block_popups",
            True
        ):
            return None

        new_tab = self.browser_window.add_tab(
            "",
            switch=True
        )

        return new_tab.browser.page()


class BrowserTab:

    def __init__(
        self,
        browser_window,
        url=""
    ):
        self.browser_window = browser_window
        self.pinned = False
        self.pending_url = ""
        self.loading = False
        self.progress = 0

        self.browser = QWebEngineView()
        self.browser.setStyleSheet("background-color: #101116;")

        self.page = BrowserPage(
            browser_window,
            self.browser
        )

        self.browser.setPage(
            self.page
        )

        self.browser.setZoomFactor(browser_window.settings.get('default_zoom', 100) / 100)

        self.browser.urlChanged.connect(
            self.url_changed
        )

        self.browser.titleChanged.connect(
            self.title_changed
        )

        self.browser.iconChanged.connect(
            self.icon_changed
        )

        self.browser.loadStarted.connect(
            self.load_started
        )

        self.browser.loadProgress.connect(self.load_progress)
        self.page.linkHovered.connect(self.link_hovered)
        self.page.audioMutedChanged.connect(lambda _: self.title_changed(self.browser.title()))

        self.browser.loadFinished.connect(
            self.load_finished
        )

        if url:
            self.browser.setUrl(
                QUrl(url)
            )

    def url_changed(self, url):
        if self.browser_window.current_tab() == self:
            self.browser_window.update_address_bar(
                self
            )

            self.browser_window.update_bookmark_button()
            self.browser_window.update_navigation_state()
        self.browser_window.schedule_session_save()

    def activate(self):
        if self.pending_url:
            url, self.pending_url = self.pending_url, ""
            self.browser.setUrl(QUrl(url))

    def session_url(self):
        return self.pending_url or self.browser.url().toString()

    def title_changed(self, title):
        index = self.browser_window.tabs.indexOf(
            self.browser
        )

        if index >= 0:
            if title:
                short_title = title[:35]
                if self.page.isAudioMuted():
                    short_title = tr("[Muet] ") + short_title
                self.browser_window.tabs.setTabToolTip(index, title)

                prefix = tr("[Épinglé] ")if self.pinned else ""

                self.browser_window.tabs.setTabText(
                    index,
                    prefix + short_title
                )

    def icon_changed(self, icon):
        index = self.browser_window.tabs.indexOf(
            self.browser
        )

        if index >= 0:
            self.browser_window.tabs.setTabIcon(
                index,
                icon
            )

    def load_started(self):
        self.loading = True
        self.progress = 0
        if self.browser_window.current_tab() == self:
            self.browser_window.update_navigation_state()

    def load_progress(self, progress):
        self.progress = progress
        if self.browser_window.current_tab() == self:
            self.browser_window.update_navigation_state()

    def link_hovered(self, url):
        if self.browser_window.current_tab() == self:
            self.browser_window.statusBar().showMessage(url)

    def load_finished(self, ok):
        self.loading = False
        self.progress = 100
        if self.browser_window.current_tab() == self:
            self.browser_window.update_navigation_state()
        self.browser_window.schedule_session_save()
        if not ok:
            return

        url = self.browser.url().toString()

        if self.browser_window.settings.get("remember_history", True) and (url.startswith(
            "http://"
        ) or url.startswith(
            "https://"
        )):
            title = self.browser.title()

            self.browser_window.storage.add_history(
                title,
                url
            )

        self.browser_window.schedule_session_save()


class BrowserTabBar(QTabBar):

    middleClicked = pyqtSignal(int)

    def mousePressEvent(self, event):

        if event.button().name == "MiddleButton":

            index = self.tabAt(
                event.position().toPoint()
            )

            if index >= 0:
                self.middleClicked.emit(
                    index
                )

        super().mousePressEvent(event)
