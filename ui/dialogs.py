from core.i18n import tr
from PyQt6.QtCore import Qt, QTimer
from PyQt6.QtWidgets import (
    QDialog, QHBoxLayout, QLabel, QListWidget, QListWidgetItem,
    QPushButton, QLineEdit, QVBoxLayout, QMessageBox,
)
from ui.dialog_style import panel_layout


class CollectionDialog(QDialog):
    """Consistent, searchable history and bookmark panels."""
    def __init__(self, browser, parent=None, bookmarks=False):
        super().__init__(parent)
        self.browser = browser
        self.bookmarks = bookmarks
        title = tr(tr('Favoris') if bookmarks else tr('Historique'))
        self.setWindowTitle('Open Kitten — ' + title)
        self.resize(800, 580)
        layout = panel_layout(self)
        heading = QLabel(title)
        heading.setObjectName('panelTitle')
        layout.addWidget(heading)
        hint = QLabel(tr('Double-cliquez pour ouvrir un élément.'))
        hint.setObjectName('panelHint')
        layout.addWidget(hint)
        self.search = QLineEdit()
        self.search.setClearButtonEnabled(True)
        self.search.setPlaceholderText(tr(tr('Rechercher dans les favoris...') if bookmarks else tr("Rechercher dans l'historique...")))
        layout.addWidget(self.search)
        self.list = QListWidget()
        self.list.setWordWrap(False)
        self.list.setUniformItemSizes(True)
        layout.addWidget(self.list, 1)
        self.empty = QLabel(tr('Aucun résultat'))
        self.empty.setObjectName('panelHint')
        layout.addWidget(self.empty)
        buttons = QHBoxLayout()
        self.open_button = QPushButton(tr('Ouvrir'))
        self.delete_button = QPushButton(tr('Supprimer'))
        buttons.addWidget(self.open_button)
        buttons.addWidget(self.delete_button)
        if not bookmarks:
            clear_button = QPushButton(tr('Tout effacer'))
            clear_button.clicked.connect(self.clear_all)
            buttons.addWidget(clear_button)
        buttons.addStretch()
        close_button = QPushButton(tr('Fermer'))
        close_button.clicked.connect(self.accept)
        buttons.addWidget(close_button)
        layout.addLayout(buttons)
        self.timer = QTimer(self)
        self.timer.setSingleShot(True)
        self.timer.setInterval(150)
        self.timer.timeout.connect(self.refresh)
        self.search.textChanged.connect(lambda: self.timer.start())
        self.list.itemDoubleClicked.connect(self.open_item)
        self.list.itemSelectionChanged.connect(self.update_actions)
        self.open_button.clicked.connect(lambda: self.open_item(self.list.currentItem()))
        self.delete_button.clicked.connect(self.delete_selected)
        self.refresh()

    def refresh(self):
        query = self.search.text().casefold()
        self.list.clear()
        if self.bookmarks:
            entries = [(item, folder) for folder, items in self.browser.storage.load_bookmarks().items() for item in items]
        else:
            entries = [(item, '') for item in self.browser.storage.load_history()]
        for entry, folder in entries:
            title, url = entry.get('title') or entry['url'], entry['url']
            if query and query not in (title + ' ' + url + ' ' + folder).casefold():
                continue
            row = QListWidgetItem(title + '\n' + (folder + ' · ' if folder else '') + url)
            row.setData(Qt.ItemDataRole.UserRole, url)
            self.list.addItem(row)
        self.empty.setVisible(self.list.count() == 0)
        self.update_actions()

    def update_actions(self):
        selected = self.list.currentItem() is not None
        self.open_button.setEnabled(selected)
        self.delete_button.setEnabled(selected)

    def open_item(self, item):
        if item is not None:
            self.browser.add_tab(item.data(Qt.ItemDataRole.UserRole), switch=True)

    def delete_selected(self):
        item = self.list.currentItem()
        if item is None:
            return
        url = item.data(Qt.ItemDataRole.UserRole)
        if self.bookmarks:
            self.browser.storage.remove_bookmark(url)
            self.browser.update_bookmark_button()
            self.browser.refresh_bookmark_bar()
        else:
            self.browser.storage.remove_history_item(url)
        self.refresh()

    def clear_all(self):
        if QMessageBox.question(self, tr('Confirmer'), tr('Effacer définitivement tout l’historique ?'),
                                QMessageBox.StandardButton.Yes | QMessageBox.StandardButton.No,
                                QMessageBox.StandardButton.No) == QMessageBox.StandardButton.Yes:
            self.browser.storage.clear_history()
            self.refresh()


class HistoryDialog(CollectionDialog):
    pass


class BookmarksDialog(CollectionDialog):
    def __init__(self, browser, parent=None):
        super().__init__(browser, parent, bookmarks=True)


class FindDialog(QDialog):

    def __init__(self, browser, parent=None):

        super().__init__(parent)

        self.browser = browser

        self.setWindowTitle(
            tr("Rechercher dans la page")
        )

        layout = QVBoxLayout(self)

        self.input = QLineEdit()

        self.input.setPlaceholderText(
            tr("Texte à rechercher...")
        )

        layout.addWidget(
            self.input
        )

        buttons = QHBoxLayout()

        previous = QPushButton(
            "◀"
        )

        next_button = QPushButton(
            "▶"
        )

        buttons.addWidget(
            previous
        )

        buttons.addWidget(
            next_button
        )

        layout.addLayout(
            buttons
        )

        self.input.textChanged.connect(
            self.find_text
        )

        previous.clicked.connect(
            lambda:
            self.browser.find_text(
                self.input.text(),
                True
            )
        )

        next_button.clicked.connect(
            lambda:
            self.browser.find_text(
                self.input.text(),
                False
            )
        )

    def find_text(self, text):

        self.browser.find_text(
            text,
            False
        )
