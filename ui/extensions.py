from ui.dialog_style import panel_layout
from core.i18n import tr
from PyQt6.QtCore import Qt
from PyQt6.QtWidgets import (
    QDialog,
    QFileDialog,
    QMessageBox,
    QHBoxLayout,
    QLabel,
    QListWidget,
    QListWidgetItem,
    QPushButton,
    QVBoxLayout,
)


class ExtensionsDialog(QDialog):

    def __init__(self, extension_manager, parent=None):
        super().__init__(parent)

        self.extension_manager = extension_manager

        self.setWindowTitle("Open Kitten — " + tr('Extensions'))
        self.resize(600, 450)

        layout = panel_layout(self)

        title = QLabel(tr("Extensions"))
        title.setStyleSheet("font-size: 22px; font-weight: bold;")
        layout.addWidget(title)

        hint = QLabel(tr('Extensions Open Kitten uniquement. Sélectionnez un dossier contenant manifest.json.\n'
                      'Après installation, activez l’extension puis fermez cette fenêtre pour l’appliquer.'))
        hint.setWordWrap(True)
        layout.addWidget(hint)
        native = QPushButton(tr('Extensions Chrome / Chromium…'))
        native.clicked.connect(self.show_webextensions)
        layout.addWidget(native)

        self.extensions_list = QListWidget()
        layout.addWidget(self.extensions_list)

        buttons = QHBoxLayout()

        self.toggle_button = QPushButton()
        self.install_button = QPushButton(tr('Installer depuis un dossier…'))
        self.close_button = QPushButton(tr("Fermer"))

        buttons.addWidget(self.toggle_button)
        buttons.addWidget(self.install_button)
        buttons.addStretch()
        buttons.addWidget(self.close_button)

        layout.addLayout(buttons)

        self.extensions_list.currentItemChanged.connect(
            self.update_toggle_button
        )
        self.toggle_button.clicked.connect(self.toggle_extension)
        self.install_button.clicked.connect(self.install_extension)
        self.close_button.clicked.connect(self.accept)

        self.refresh_extensions()

    def show_webextensions(self):
        from ui.webextensions import WebExtensionsDialog
        browser = self.parent()
        dialog = getattr(browser, 'webextensions_dialog', None)
        if dialog is None:
            dialog = WebExtensionsDialog(browser, browser)
            browser.webextensions_dialog = dialog
        else:
            if dialog.manager:
                dialog.refresh()
        self.accept()
        dialog.show()
        dialog.raise_()
        dialog.activateWindow()

    def install_extension(self):
        folder = QFileDialog.getExistingDirectory(self, tr('Choisir le dossier de l’extension'))
        if not folder:
            return
        try:
            extension_id = self.extension_manager.install_from_folder(folder)
        except (OSError, ValueError, UnicodeError) as error:
            QMessageBox.warning(self, tr('Installation impossible'), str(error))
            return
        self.refresh_extensions()
        for row in range(self.extensions_list.count()):
            item = self.extensions_list.item(row)
            data = item.data(Qt.ItemDataRole.UserRole)
            if data and data['id'] == extension_id:
                self.extensions_list.setCurrentRow(row)
                break
        QMessageBox.information(self, tr('Extension installée'),
                                tr('L’extension a été copiée et est désactivée. Cliquez sur Activer pour l’utiliser.'))

    def refresh_extensions(self):
        self.extensions_list.clear()

        for extension in self.extension_manager.list_extensions():
            builtin = extension['id'] in ('ad_blocker', 'privacy_guard', 'dark_mode')
            name = tr(extension['name']) if builtin else extension['name']
            description = tr(extension['description']) if builtin else extension['description']
            status = tr("Activée") if extension["enabled"] else tr("Désactivée")

            item = QListWidgetItem(
                f"{name} {extension['version']}\n"
                f"{description}\n{status}"
            )

            item.setData(
                Qt.ItemDataRole.UserRole,
                extension
            )

            self.extensions_list.addItem(item)

        if self.extensions_list.count():
            self.extensions_list.setCurrentRow(0)
        else:
            empty = QListWidgetItem(
                tr("Aucune extension installée.")
            )
            empty.setFlags(Qt.ItemFlag.NoItemFlags)
            self.extensions_list.addItem(empty)

        self.update_toggle_button()

    def update_toggle_button(self, current=None, previous=None):
        item = self.extensions_list.currentItem()
        extension = (
            item.data(Qt.ItemDataRole.UserRole)
            if item
            else None
        )

        if not extension:
            self.toggle_button.setEnabled(False)
            self.toggle_button.setText(tr("Activer / Désactiver"))
            return

        self.toggle_button.setEnabled(True)
        self.toggle_button.setText(
            tr("Désactiver") if extension["enabled"] else tr("Activer")
        )

    def toggle_extension(self):
        item = self.extensions_list.currentItem()

        if item is None:
            return

        extension = item.data(Qt.ItemDataRole.UserRole)

        if not extension:
            return

        if not extension['enabled'] and extension.get('script'):
            answer = QMessageBox.question(self, tr('Activer cette extension ?'),
                tr('Son script peut lire et modifier les pages visitées et envoyer des données sur Internet. '
                'Activez uniquement les extensions dont vous connaissez la provenance.'),
                QMessageBox.StandardButton.Yes | QMessageBox.StandardButton.No,
                QMessageBox.StandardButton.No)
            if answer != QMessageBox.StandardButton.Yes:
                return

        try:
            self.extension_manager.set_enabled(extension['id'], not extension['enabled'])
        except OSError as error:
            QMessageBox.warning(self, tr('Enregistrement impossible'), str(error))
            return

        self.refresh_extensions()
