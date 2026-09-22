from core.i18n import tr
import os

from PyQt6.QtCore import (
    QUrl,
    Qt,
    QTimer,
)
from PyQt6.QtGui import (
    QAction,
    QKeySequence,
)
from PyQt6.QtWidgets import (
    QHBoxLayout,
    QProgressBar,
    QApplication,
    QFrame,
    QLabel,
    QLineEdit,
    QMainWindow,
    QMenu,
    QMessageBox,
    QPushButton,
    QTabWidget,
    QToolButton,
    QVBoxLayout,
    QWidget,
)
from PyQt6.QtWebEngineCore import (
    QWebEngineProfile,
    QWebEnginePage,
    QWebEngineSettings,
)
from PyQt6.QtWebEngineWidgets import (
    QWebEngineView,
)


from core.navigation import resolve_address
from core.storage import Storage
from core.profiles import ProfileManager
from core.privacy import PrivacyManager
from core.extensions import ExtensionManager
from core.app_paths import RESOURCE_DIR, VERSION
from core.web_settings import apply_web_settings
from ui.icons import icon
from ui.theme import stylesheet

from ui.tabs import (
    BrowserTab,
    BrowserTabBar,
)

from ui.dialogs import (
    HistoryDialog,
    BookmarksDialog,
    FindDialog,
)

from ui.downloads import (
    DownloadsDialog,
)

from ui.settings import (
    SettingsDialog,
)

from ui.profiles import (
    ProfilesDialog,
)

from ui.private import (
    PrivateBrowser,
)

from ui.extensions import (
    ExtensionsDialog,
)


class MiniBrowser(QMainWindow):

    def __init__(
        self,
        settings
    ):
        super().__init__()

        self.settings = settings

        base_dir = str(RESOURCE_DIR)
        self.data_dir = self.settings.data_dir

        self.profile_manager = ProfileManager(
            self.data_dir
        )

        self.profile_name = (
            self.settings.get(
                "default_profile",
                "Default"
            )
        )

        self.profile_manager.create_profile(
            self.profile_name
        )

        self.profile_dir = (
            self.profile_manager.get_profile_dir(
                self.profile_name
            )
        )

        self.storage = Storage(
            self.profile_dir
        )

        self.privacy = PrivacyManager(
            self
        )

        extensions_dir = os.path.join(
            base_dir,
            "extensions"
        )

        self.extension_manager = (
            ExtensionManager(
                extensions_dir,
                os.path.join(self.data_dir, "extensions-state.json"),
                settings=self.settings
            )
        )

        self.private_windows = []

        self.tabs = None
        self.browser_tabs = []
        self.closed_tabs = []

        self.session_save_timer = QTimer(self)
        self.session_save_timer.setSingleShot(True)
        self.session_save_timer.timeout.connect(
            self.save_session
        )

        self.setWindowTitle(
            "Open Kitten " + VERSION
        )

        self.resize(
            1400,
            900
        )

        self.setup_profile()
        self.setup_ui()
        self.setup_shortcuts()
        self.apply_settings()

        self.restore_or_create_session()

    # =========================================================
    # PROFILE WEBENGINE
    # =========================================================

    def setup_profile(self):

        self.profile = QWebEngineProfile(self.profile_name, self)
        self.profile.setPersistentStoragePath(
            os.path.join(self.profile_dir, "webengine"))
        self.profile.setCachePath(os.path.join(self.profile_dir, "cache"))

        self.profile.downloadRequested.connect(
            self.handle_download
        )

        self.profile.settings().setAttribute(
            QWebEngineSettings.WebAttribute.DnsPrefetchEnabled,
            True
        )

        self.extension_manager.apply_to_profile(
            self.profile
        )

    # =========================================================
    # UI
    # =========================================================

    def setup_ui(self):

        central = QWidget()

        main_layout = QVBoxLayout(
            central
        )

        main_layout.setContentsMargins(
            0,
            0,
            0,
            0
        )

        main_layout.setSpacing(0)

        # -----------------------------------------------------
        # NAVIGATION BAR
        # -----------------------------------------------------

        nav = QWidget()
        nav.setObjectName("navigationBar")

        nav_layout = QHBoxLayout(nav)

        nav_layout.setContentsMargins(
            6,
            6,
            6,
            6
        )

        self.back_button = QPushButton(
            "←"
        )

        self.forward_button = QPushButton(
            "→"
        )

        self.reload_button = QPushButton(
            "↻"
        )

        self.home_button = QPushButton(
            "⌂"
        )

        self.private_button = QPushButton(
            ""
        )

        self.security_label = QLabel(
            ""
        )
        self.security_label.setObjectName("securityLabel")

        self.address_bar = QLineEdit()

        self.address_bar.setPlaceholderText(
            tr("Rechercher ou saisir une adresse...")
        )

        self.shields_button = QToolButton()
        self.shields_button.setObjectName("shieldsButton")
        self.shields_button.setCheckable(True)
        self.shields_button.setText("")

        self.bookmark_button = QPushButton(
            "☆"
        )

        self.menu_button = QToolButton()

        self.menu_button.setText(
            ""
        )

        nav_layout.addWidget(
            self.back_button
        )

        nav_layout.addWidget(
            self.forward_button
        )

        nav_layout.addWidget(
            self.reload_button
        )

        nav_layout.addWidget(
            self.home_button
        )

        nav_layout.addWidget(
            self.private_button
        )

        nav_layout.addWidget(
            self.security_label
        )

        nav_layout.addWidget(
            self.address_bar,
            1
        )

        nav_layout.addWidget(
            self.shields_button
        )

        nav_layout.addWidget(
            self.bookmark_button
        )

        nav_layout.addWidget(
            self.menu_button
        )

        main_layout.addWidget(
            nav
        )

        self.load_progress = QProgressBar()
        self.load_progress.setObjectName("pageLoadProgress")
        self.load_progress.setRange(0, 100)
        self.load_progress.setTextVisible(False)
        self.load_progress.setFixedHeight(2)
        self.load_progress.setValue(0)
        main_layout.addWidget(self.load_progress)

        # -----------------------------------------------------
        # BOOKMARK BAR
        # -----------------------------------------------------

        self.bookmark_bar = QWidget()
        self.bookmark_bar.setObjectName("bookmarkBar")

        self.bookmark_layout = QHBoxLayout(
            self.bookmark_bar
        )

        self.bookmark_layout.setContentsMargins(
            6,
            3,
            6,
            3
        )

        self.bookmark_layout.setSpacing(
            4
        )

        main_layout.addWidget(
            self.bookmark_bar
        )

        # -----------------------------------------------------
        # TABS
        # -----------------------------------------------------

        self.tabs = QTabWidget()
        self.tabs.setObjectName("browserTabs")

        self.tab_bar = BrowserTabBar()

        self.tabs.setTabBar(
            self.tab_bar
        )

        self.tabs.setTabsClosable(
            True
        )

        self.tabs.setMovable(
            True
        )
        self.tab_bar.tabMoved.connect(self.sync_tab_order)

        self.tabs.setDocumentMode(
            True
        )
        self.tab_bar.setExpanding(False)
        self.tab_bar.setElideMode(Qt.TextElideMode.ElideRight)
        self.tab_bar.setUsesScrollButtons(True)

        self.sidebar_new_tab = QToolButton()
        self.sidebar_new_tab.setToolTip(tr('Nouvel onglet (Ctrl+T)'))
        self.sidebar_new_tab.setAccessibleName(tr('Nouvel onglet'))
        self.tabs.setCornerWidget(self.sidebar_new_tab, Qt.Corner.TopRightCorner)
        main_layout.addWidget(self.tabs, 1)

        self.setCentralWidget(
            central
        )
        self.statusBar().setSizeGripEnabled(False)

        # -----------------------------------------------------
        # SIGNALS
        # -----------------------------------------------------

        self.back_button.clicked.connect(
            self.go_back
        )

        self.forward_button.clicked.connect(
            self.go_forward
        )

        self.reload_button.clicked.connect(
            self.reload_or_stop
        )

        self.home_button.clicked.connect(
            self.go_home
        )

        self.private_button.clicked.connect(
            self.open_private_window
        )

        self.address_bar.returnPressed.connect(
            self.navigate
        )

        self.shields_button.clicked.connect(
            self.toggle_protections
        )

        self.bookmark_button.clicked.connect(
            self.toggle_bookmark
        )

        self.menu_button.clicked.connect(
            self.show_menu
        )

        self.sidebar_new_tab.clicked.connect(
            lambda: self.add_tab("", True)
        )

        self.tabs.currentChanged.connect(
            self.tab_changed
        )

        self.tabs.tabCloseRequested.connect(
            self.close_tab
        )

        self.tab_bar.middleClicked.connect(
            self.close_tab
        )

        self.tabs.tabBar().setContextMenuPolicy(
            Qt.ContextMenuPolicy.CustomContextMenu
        )

        self.tabs.tabBar().customContextMenuRequested.connect(
            self.tab_context_menu
        )

        self.update_shields_button()

    # =========================================================
    # SHORTCUTS
    # =========================================================

    def setup_shortcuts(self):

        actions = []

        def shortcut(
            key,
            callback
        ):
            action = QAction(
                self
            )

            action.setShortcut(
                QKeySequence(key)
            )

            action.triggered.connect(
                callback
            )

            self.addAction(
                action
            )

            actions.append(
                action
            )

        shortcut("Ctrl+Shift+T", self.restore_closed_tab)
        shortcut("Ctrl+Tab", lambda: self.select_relative_tab(1))
        shortcut("Ctrl+Shift+Tab", lambda: self.select_relative_tab(-1))
        shortcut("Alt+Left", self.go_back)
        shortcut("Alt+Right", self.go_forward)
        shortcut("F5", self.reload_page)
        shortcut("Escape", self.stop_loading)

        shortcut(
            "Ctrl+L",
            self.focus_address
        )

        shortcut(
            "Ctrl+T",
            lambda:
            self.add_tab(
                "",
                True
            )
        )

        shortcut(
            "Ctrl+W",
            lambda:
            self.close_tab(
                self.tabs.currentIndex()
            )
        )

        shortcut(
            "Ctrl+F",
            self.show_find
        )

        shortcut(
            "Ctrl++",
            lambda:
            self.change_zoom(
                0.1
            )
        )

        shortcut(
            "Ctrl+=",
            lambda:
            self.change_zoom(
                0.1
            )
        )

        shortcut(
            "Ctrl+-",
            lambda:
            self.change_zoom(
                -0.1
            )
        )

        shortcut(
            "Ctrl+0",
            lambda:
            self.set_zoom(
                1.0
            )
        )

        shortcut(
            "F11",
            self.toggle_fullscreen
        )

    # =========================================================
    # TABS
    # =========================================================

    def current_tab(self):
        widget = self.tabs.currentWidget()
        return next((tab for tab in self.browser_tabs
                     if tab.browser is widget), None)

    def sync_tab_order(self, *args):
        by_widget = {tab.browser: tab for tab in self.browser_tabs}
        self.browser_tabs[:] = [
            by_widget[self.tabs.widget(index)]
            for index in range(self.tabs.count())
        ]
        self.tab_changed(self.tabs.currentIndex())

    def add_tab(
        self,
        url="",
        switch=True,
        deferred=False
    ):

        tab = BrowserTab(
            self,
            ""
        )
        tab.pending_url = url if deferred else ""

        # Register before addTab emits currentChanged for the first tab.
        self.browser_tabs.append(tab)

        index = self.tabs.addTab(
            tab.browser,
            tr("Nouvel onglet")
        )

        if switch:
            self.tabs.setCurrentIndex(
                index
            )

        if url and not deferred:
            tab.browser.setUrl(QUrl(url))
        elif deferred:
            self.tabs.setTabText(index, QUrl(url).host() or tr("Onglet en attente"))
        else:
            self.show_new_tab(
                tab
            )

        self.update_bookmark_button()

        return tab

    def close_tab(self, index):

        if index < 0 or index >= self.tabs.count():
            return

        if self.browser_tabs[index].pinned:
            return

        url = self.browser_tabs[index].session_url()
        if url.startswith(('http://', 'https://')):
            self.closed_tabs.append(url)
            self.closed_tabs = self.closed_tabs[-30:]

        if self.tabs.count() <= 1:

            tab = self.current_tab()

            if tab:
                self.show_new_tab(
                    tab
                )

            return

        tab = self.browser_tabs.pop(index)
        tab.browser.stop()
        self.tabs.removeTab(index)
        tab.browser.deleteLater()
        self.tab_changed(self.tabs.currentIndex())

        self.save_session()

    def restore_closed_tab(self):
        if self.closed_tabs:
            self.add_tab(self.closed_tabs.pop(), True)

    # =========================================================
    # NAVIGATION
    # =========================================================

    def go_back(self):

        tab = self.current_tab()

        if tab:
            tab.browser.back()

    def go_forward(self):

        tab = self.current_tab()

        if tab:
            tab.browser.forward()

    def reload_page(self):

        tab = self.current_tab()

        if tab:
            tab.browser.reload()

    def navigate(self):
        url = resolve_address(self.address_bar.text(), self.settings.get('search_engine', 'Google'))
        tab = self.current_tab()
        if url and tab:
            tab.pending_url = ""
            tab.browser.setUrl(QUrl(url))
            tab.browser.setFocus()

    def select_relative_tab(self, offset):
        if self.tabs.count():
            self.tabs.setCurrentIndex((self.tabs.currentIndex() + offset) % self.tabs.count())

    def stop_loading(self):
        tab = self.current_tab()
        if tab and tab.loading:
            tab.browser.stop()

    def reload_or_stop(self):
        tab = self.current_tab()
        if tab:
            tab.browser.stop() if tab.loading else tab.browser.reload()

    def update_navigation_state(self):
        tab = self.current_tab()
        loading = bool(tab and tab.loading)
        self.back_button.setEnabled(bool(tab and tab.browser.history().canGoBack()))
        self.forward_button.setEnabled(bool(tab and tab.browser.history().canGoForward()))
        self.reload_button.setIcon(icon('stop' if loading else 'reload', self.icon_color()))
        label = tr('Arrêter le chargement (Échap)') if loading else tr('Actualiser (F5)')
        self.reload_button.setToolTip(label)
        self.reload_button.setAccessibleName(label)
        self.load_progress.setValue(tab.progress if loading else 0)

    def focus_address(self):

        self.address_bar.setFocus()
        self.address_bar.selectAll()

    # =========================================================
    # HOME
    # =========================================================

    def go_home(self):

        tab = self.current_tab()

        if not tab:
            return

        homepage = self.settings.get(
            "homepage",
            "newtab"
        )

        if homepage == "custom":

            url = self.settings.get(
                "custom_homepage",
                ""
            )

            if url:
                tab.browser.setUrl(
                    QUrl(url)
                )
                return

        self.show_new_tab(
            tab
        )

    def show_new_tab(self, tab):
        tab.pending_url = ""
        from core.homepage import render_homepage
        logo = (RESOURCE_DIR / 'assets' / 'icon.svg').read_text(encoding='utf-8')
        tab.browser.setHtml(render_homepage(self.settings, self.storage, logo))

    # =========================================================
    # ADDRESS BAR
    # =========================================================

    def update_address_bar(
        self,
        tab
    ):

        if tab != self.current_tab():
            return

        url = tab.browser.url().toString()

        # La page « nouvel onglet » est chargée avec setHtml(). QtWebEngine
        # lui attribue alors une longue URL interne data:text/html..., qui ne
        # doit pas apparaître dans la barre d'adresse.
        if url.startswith("data:text/html"):
            self.address_bar.clear()
        else:
            self.address_bar.setText(
                url
            )

        symbol = 'lock' if url.startswith('https://') else ('warning' if url.startswith('http://') else 'info')
        self.security_label.setPixmap(icon(symbol, self.icon_color()).pixmap(18, 18))
        self.security_label.setToolTip(tr('Connexion HTTPS') if symbol == 'lock' else
                                      (tr('Connexion HTTP non chiffrée') if symbol == 'warning' else tr('Page interne')))

    # =========================================================
    # BOOKMARKS
    # =========================================================

    def toggle_bookmark(self):

        tab = self.current_tab()

        if not tab:
            return

        url = tab.browser.url().toString()

        if not url.startswith(
            ("http://", "https://")
        ):
            return

        if self.storage.is_bookmarked(
            url
        ):

            self.storage.remove_bookmark(
                url
            )

        else:

            title = tab.browser.title()

            self.storage.add_bookmark(
                title,
                url,
                "Unsorted"
            )

        self.update_bookmark_button()
        self.refresh_bookmark_bar()

    def update_bookmark_button(self):
        tab = self.current_tab()
        saved = bool(tab and self.storage.is_bookmarked(tab.browser.url().toString()))
        self.bookmark_button.setText('')
        self.bookmark_button.setIcon(icon('star', self.icon_color(), filled=saved))
        self.bookmark_button.setToolTip(tr('Retirer des favoris') if saved else tr('Ajouter aux favoris'))

    def refresh_bookmark_bar(self):

        while self.bookmark_layout.count():

            item = (
                self.bookmark_layout.takeAt(0)
            )

            widget = item.widget()

            if widget:
                widget.deleteLater()

        bookmarks = (
            self.storage.load_bookmarks()
        )

        for folder, items in bookmarks.items():

            for bookmark in items:

                button = QPushButton(
                    " "
                    + bookmark.get(
                        "title",
                        tr("Favori")
                    )[:25]
                )

                url = bookmark.get(
                    "url",
                    ""
                )

                button.setToolTip(
                    url
                )

                button.clicked.connect(
                    lambda checked=False,
                    u=url:
                    self.add_tab(
                        u,
                        True
                    )
                )

                self.bookmark_layout.addWidget(
                    button
                )

        self.bookmark_layout.addStretch()

    # =========================================================
    # DOWNLOADS
    # =========================================================

    def handle_download(
        self,
        download
    ):

        directory = self.settings.get(
            "download_directory",
            ""
        )

        if not directory:
            directory = os.path.join(
                os.path.expanduser("~"),
                "Downloads"
            )

        os.makedirs(
            directory,
            exist_ok=True
        )

        filename = (
            download.suggestedFileName()
            or "download"
        )

        base, extension = os.path.splitext(
            filename
        )

        final_path = os.path.join(
            directory,
            filename
        )

        counter = 1

        while os.path.exists(
            final_path
        ):

            final_path = os.path.join(
                directory,
                f"{base} ({counter}){extension}"
            )

            counter += 1

        final_filename = os.path.basename(
            final_path
        )

        download.setDownloadDirectory(
            directory
        )

        download.setDownloadFileName(
            final_filename
        )

        download.accept()

        if not hasattr(
            self,
            "downloads_dialog"
        ):
            self.downloads_dialog = (
                DownloadsDialog(self)
            )

        self.downloads_dialog.add_download(
            download
        )

    # =========================================================
    # DIALOGS
    # =========================================================

    def show_history(self):

        dialog = HistoryDialog(
            self,
            self
        )

        dialog.exec()

    def show_bookmarks(self):

        dialog = BookmarksDialog(
            self,
            self
        )

        dialog.exec()

    def show_downloads(self):

        if not hasattr(
            self,
            "downloads_dialog"
        ):
            self.downloads_dialog = DownloadsDialog(self)

        self.downloads_dialog.show()
        self.downloads_dialog.raise_()
        self.downloads_dialog.activateWindow()

    def show_find(self):

        dialog = FindDialog(
            self,
            self
        )

        dialog.show()
        dialog.activateWindow()
        dialog.raise_()

    def show_settings(self):

        dialog = SettingsDialog(
            self,
            self
        )

        dialog.exec()

    def show_profiles(self):

        dialog = ProfilesDialog(
            self,
            self
        )

        dialog.exec()

    def show_extensions(self):
        if getattr(self, 'extensions_dialog', None) is not None:
            self.extensions_dialog.show()
            self.extensions_dialog.raise_()
            self.extensions_dialog.activateWindow()
            return
        self.extensions_dialog = ExtensionsDialog(self.extension_manager, self)
        self.extensions_dialog.finished.connect(self.extensions_closed)
        self.extensions_dialog.show()

    def extensions_closed(self, result):
        dialog = self.extensions_dialog
        self.extensions_dialog = None
        dialog.deleteLater()
        self.extension_manager.apply_to_profile(
            self.profile
        )

        self.update_shields_button()

        for tab in self.browser_tabs:
            tab.browser.reload()

    def update_shields_button(self):

        protected_ids = {"ad_blocker", "privacy_guard"}

        enabled = any(
            extension["enabled"]
            for extension in self.extension_manager.list_extensions()
            if extension["id"] in protected_ids
        )

        self.shields_button.setText('')
        self.shields_button.setChecked(enabled)
        self.shields_button.setIcon(icon('shield', self.icon_color()))
        if enabled:
            self.shields_button.setToolTip(
                tr("Protections activées — cliquer pour désactiver")
            )
        else:
            self.shields_button.setToolTip(
                tr("Protections désactivées — cliquer pour activer")
            )

    def toggle_protections(self):

        protected_ids = {"ad_blocker", "privacy_guard"}

        protected_extensions = [
            extension
            for extension in self.extension_manager.list_extensions()
            if extension["id"] in protected_ids
        ]

        enabled = any(
            extension["enabled"]
            for extension in protected_extensions
        )

        for extension in protected_extensions:
            self.extension_manager.set_enabled(
                extension["id"],
                not enabled
            )

        self.extension_manager.apply_to_profile(
            self.profile
        )
        self.update_shields_button()

        for tab in self.browser_tabs:
            tab.browser.reload()

    # =========================================================
    # FIND
    # =========================================================

    def find_text(
        self,
        text,
        backwards=False
    ):

        tab = self.current_tab()

        if not tab:
            return

        tab.browser.findText(
            text,
            QWebEnginePage.FindFlag.FindBackward
            if backwards
            else QWebEnginePage.FindFlag(0)
        )

    # =========================================================
    # ZOOM
    # =========================================================

    def change_zoom(
        self,
        amount
    ):

        tab = self.current_tab()

        if not tab:
            return

        zoom = tab.browser.zoomFactor()

        zoom += amount

        zoom = max(
            0.25,
            min(
                5.0,
                zoom
            )
        )

        tab.browser.setZoomFactor(
            zoom
        )

    def set_zoom(
        self,
        value
    ):

        tab = self.current_tab()

        if tab:
            tab.browser.setZoomFactor(
                value
            )

    # =========================================================
    # FULLSCREEN
    # =========================================================

    def toggle_fullscreen(self):

        if self.isFullScreen():
            self.showNormal()
        else:
            self.showFullScreen()

    # =========================================================
    # PRIVATE MODE
    # =========================================================

    def open_private_window(self):

        window = PrivateBrowser(
            self
        )

        self.private_windows.append(
            window
        )

        window.closed.connect(
            self.forget_private_window
        )

        window.show()

    def forget_private_window(self, window):

        if window in self.private_windows:
            self.private_windows.remove(window)

    # =========================================================
    # PROFILES
    # =========================================================

    def save_session(self, force=False):

        if not force and not self.settings.get(
            "autosave_session",
            True
        ):
            return

        urls = []

        for tab in self.browser_tabs:

            url = tab.session_url()

            if url.startswith(
                ("http://", "https://")
            ):
                urls.append(url)

        self.storage.save_session(
            urls
        )

    def schedule_session_save(self):

        if not self.settings.get(
            "autosave_session",
            True
        ):
            return

        # Regroupe les chargements successifs en une seule écriture disque.
        self.session_save_timer.start(1000)

    def restore_or_create_session(self):

        if self.settings.get(
            "restore_session",
            True
        ):

            urls = self.storage.load_session()

            if urls:

                for index, url in enumerate(
                    urls
                ):
                    self.add_tab(
                        url,
                        switch=(index == 0),
                        deferred=index > 0 and self.settings.get("lazy_restore", True)
                    )

                return

        self.add_tab(
            "",
            True
        )

    # =========================================================
    # TAB CONTEXT MENU
    # =========================================================

    def tab_context_menu(
        self,
        position
    ):

        index = self.tab_bar.tabAt(
            position
        )

        if index < 0:
            return

        menu = QMenu(
            self
        )

        new_tab = menu.addAction(
            tr("Nouvel onglet")
        )

        duplicate = menu.addAction(
            tr("Dupliquer l'onglet")
        )

        tab = self.browser_tabs[index]

        pin_tab = menu.addAction(
            tr("Retirer l'épingle")
            if tab.pinned
            else tr("Épingler l'onglet")
        )

        mute = menu.addAction(tr('Rétablir le son') if tab.page.isAudioMuted() else tr('Couper le son'))
        copy_link = menu.addAction(tr('Copier l’adresse'))
        copy_link.setEnabled(tab.session_url().startswith(('http://', 'https://')))

        menu.addSeparator()

        close = menu.addAction(
            tr("Fermer")
        )

        close_others = menu.addAction(
            tr("Fermer les autres")
        )

        menu.addSeparator()

        reopen = menu.addAction(
            tr("Restaurer un onglet")
        )

        close.setEnabled(not tab.pinned)
        reopen.setEnabled(bool(self.closed_tabs))

        action = menu.exec(
            self.tab_bar.mapToGlobal(
                position
            )
        )

        if action == new_tab:

            self.add_tab(
                "",
                True
            )

        elif action == duplicate:

            url = tab.session_url()
            if url.startswith("data:"):
                url = ""

            self.add_tab(
                url,
                True
            )

        elif action == mute:
            tab.page.setAudioMuted(not tab.page.isAudioMuted())

        elif action == copy_link:
            QApplication.clipboard().setText(tab.session_url())

        elif action == pin_tab:

            tab.pinned = not tab.pinned

            title = tab.browser.title() or tr("Nouvel onglet")

            self.tabs.setTabText(
                index,
                (tr("[Épinglé] ")if tab.pinned else "") + title[:35]
            )

        elif action == close:

            self.close_tab(
                index
            )

        elif action == close_others:

            for i in reversed(
                range(
                    len(self.browser_tabs)
                )
            ):

                if i != index:
                    self.close_tab(i)

        elif action == reopen:

            self.restore_closed_tab()

    # =========================================================
    # MENU
    # =========================================================

    def show_menu(self):
        menu = QMenu(self)
        entries = [
            (tr('Favoris'), self.show_bookmarks),
            (tr('Historique'), self.show_history),
            (tr('Téléchargements'), self.show_downloads),
            None,
            (tr('Nouvelle fenêtre privée'), self.open_private_window),
            (tr('Profils'), self.show_profiles),
            (tr('Extensions'), self.show_extensions),
            None,
            (tr('Paramètres'), self.show_settings),
            (tr('Plein écran (F11)'), self.toggle_fullscreen),
            (tr('À propos'), self.show_about),
        ]
        actions = {}
        for entry in entries:
            if entry is None:
                menu.addSeparator()
            else:
                label, callback = entry
                actions[menu.addAction(label)] = callback
        chosen = menu.exec(self.menu_button.mapToGlobal(self.menu_button.rect().bottomLeft()))
        if chosen in actions:
            actions[chosen]()

    def show_about(self):
        QMessageBox.about(self, 'Open Kitten',
                          'Open Kitten ' + VERSION + tr('\nNavigateur expérimental basé sur QtWebEngine.'))

    # =========================================================
    # SETTINGS
    # =========================================================

    def apply_settings(self):
        apply_web_settings(self.profile, self.settings)
        for tab in self.browser_tabs:
            tab.browser.setZoomFactor(self.settings.get('default_zoom', 100) / 100)

        if self.settings.get(
            "show_bookmark_bar",
            True
        ):
            self.bookmark_bar.show()
        else:
            self.bookmark_bar.hide()

        self.apply_theme()
        self.setStyleSheet(self.styleSheet() + '''
            #navigationBar QPushButton, #navigationBar QToolButton {
                border: none; background: transparent; padding: 5px; border-radius: 5px;
            }
            #navigationBar QPushButton:hover, #navigationBar QToolButton:hover {
                background: rgba(120, 140, 160, 35);
            }
            #navigationBar QToolButton:checked { background: rgba(70, 150, 190, 55); }
            QMenu { padding: 6px; }
            QMenu::item { padding: 8px 24px; }
            QTabBar::tab { min-width: 95px; max-width: 210px; padding: 8px 12px;
                border-top-left-radius: 6px; border-top-right-radius: 6px; }
        ''')
        self.refresh_chrome()
        for tab in self.browser_tabs:
            if not tab.pending_url and tab.browser.url().toString().startswith('data:text/html'):
                self.show_new_tab(tab)
        self.update_navigation_state()

        self.refresh_bookmark_bar()

    def apply_theme(self):
        self.setStyleSheet(stylesheet(self.settings.get('theme', 'dark'), self.settings))

    def icon_color(self):
        return '#c5ceda' if self.settings.get('theme', 'dark') == 'dark' else '#344454'

    def refresh_chrome(self):
        buttons = [(self.back_button, 'back', tr('Précédent')),
                   (self.forward_button, 'forward', tr('Suivant')),
                   (self.reload_button, 'reload', tr('Actualiser')),
                   (self.home_button, 'home', tr('Accueil')),
                   (self.menu_button, 'menu', tr('Menu')),
                   (self.sidebar_new_tab, 'plus', tr('Nouvel onglet (Ctrl+T)'))]
        for button, name, label in buttons:
            button.setText('')
            button.setIcon(icon(name, self.icon_color()))
            button.setFixedSize(34, 34)
            button.setToolTip(label)
            button.setAccessibleName(label)
        self.private_button.hide()
        self.home_button.setVisible(self.settings.get('show_home_button', True))
        self.statusBar().setVisible(self.settings.get('show_status_bar', True))
        for button, label in [(self.bookmark_button, tr('Favori')), (self.shields_button, tr('Protections'))]:
            button.setFixedSize(34, 34)
            button.setAccessibleName(label)
        self.update_shields_button()
        self.update_bookmark_button()
        if self.current_tab():
            self.update_address_bar(self.current_tab())
        else:
            self.security_label.setText('')
            self.security_label.setPixmap(icon('info', self.icon_color()).pixmap(18, 18))

    def tab_changed(
        self,
        index
    ):

        tab = self.current_tab()

        if not tab:
            return

        tab.activate()
        self.statusBar().clearMessage()
        self.update_navigation_state()

        self.update_address_bar(
            tab
        )

        self.update_bookmark_button()

    # =========================================================
    # CLOSE
    # =========================================================

    def closeEvent(
        self,
        event
    ):

        self.session_save_timer.stop()
        self.save_session(force=True)

        # Pages must be destroyed before their profile.
        from PyQt6 import sip
        for window in list(self.private_windows):
            window.close()
        for tab in self.browser_tabs:
            tab.browser.stop()
            sip.delete(tab.page)
        self.browser_tabs.clear()

        event.accept()
