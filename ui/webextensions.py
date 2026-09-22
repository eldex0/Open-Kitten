from ui.dialog_style import panel_layout
from core.i18n import tr
"""Optional native Qt 6.10+ Chromium extension management."""
import zipfile
from pathlib import Path
from PyQt6.QtCore import Qt, QTimer
from core.extension_diagnostics import record_extension_event
from PyQt6.QtWidgets import QDialog, QVBoxLayout, QHBoxLayout, QLabel, QPushButton, QListWidget, QListWidgetItem, QFileDialog, QMessageBox
from core.webextension_manifest import inspect_webextension


class WebExtensionsDialog(QDialog):
    def __init__(self, browser, parent=None):
        super().__init__(parent)
        self.browser = browser
        self.manager = None
        self.refresh_timer = QTimer(self)
        self.refresh_timer.setSingleShot(True)
        self.refresh_timer.timeout.connect(self.refresh)
        self.setWindowTitle(tr('Extensions Chrome / Chromium'))
        self.resize(700, 480)
        layout = panel_layout(self)
        note = QLabel(tr('Manifest V3 uniquement · Compatibilité selon les API utilisées.\n'
                      'Pour une extension Firefox, utilisez sa version Chromium.\n'
                      'Les extensions installées sont désactivées au redémarrage : activez-les ici.'))
        note.setWordWrap(True)
        layout.addWidget(note)
        self.list = QListWidget()
        layout.addWidget(self.list)
        row = QHBoxLayout()
        self.folder = QPushButton(tr('Installer un dossier…'))
        self.archive = QPushButton(tr('Installer un ZIP…'))
        self.toggle = QPushButton(tr('Activer / Désactiver'))
        self.popup = QPushButton(tr('Ouvrir l’extension'))
        self.remove = QPushButton(tr('Désinstaller'))
        for button in (self.folder, self.archive, self.toggle, self.popup, self.remove):
            row.addWidget(button)
        layout.addLayout(row)
        self.status = QLabel()
        self.status.setWordWrap(True)
        self.status.setTextFormat(Qt.TextFormat.PlainText)
        layout.addWidget(self.status)
        close = QPushButton(tr('Fermer'))
        close.clicked.connect(self.accept)
        layout.addWidget(close)
        if not hasattr(browser.profile, 'extensionManager'):
            self.status.setText(tr('Fonction indisponible : PyQt6-WebEngine et QtWebEngine 6.10 ou plus récents sont nécessaires.'))
            for button in (self.folder, self.archive, self.toggle, self.popup, self.remove):
                button.setEnabled(False)
            return
        self.manager = browser.profile.extensionManager()
        self.manager.installFinished.connect(self.completed)
        self.manager.uninstallFinished.connect(self.completed)
        self.manager.loadFinished.connect(self.completed)
        self.folder.clicked.connect(self.install_folder)
        self.archive.clicked.connect(self.install_zip)
        self.toggle.clicked.connect(self.toggle_selected)
        self.popup.clicked.connect(self.open_popup)
        self.remove.clicked.connect(self.remove_selected)
        self.list.currentItemChanged.connect(self.selection_changed)
        self.refresh()

    def selected(self):
        item = self.list.currentItem()
        if item:
            return next((e for e in self.manager.extensions()
                         if e.id() == item.data(Qt.ItemDataRole.UserRole)), None)

    def refresh(self):
        self.list.clear()
        for extension in self.manager.extensions():
            if not extension.isInstalled():
                continue  # Do not expose Chromium's internal PDF/Hangouts extensions.
            status = tr('Activée') if extension.isEnabled() else tr('Désactivée')
            item = QListWidgetItem(extension.name() + ' — ' + status)
            item.setData(Qt.ItemDataRole.UserRole, extension.id())
            self.list.addItem(item)
        if self.list.count():
            self.list.setCurrentRow(0)
        self.selection_changed()

    def selection_changed(self, *args):
        extension = self.selected()
        self.toggle.setEnabled(extension is not None)
        self.remove.setEnabled(extension is not None)
        self.popup.setEnabled(extension is not None and extension.isEnabled()
                              and not extension.actionPopupUrl().isEmpty())

    def completed(self, extension):
        self.folder.setEnabled(True)
        self.archive.setEnabled(True)
        self.status.setText(extension.error() or tr('Opération terminée. Rechargez la page après activation.'))
        # Do not re-enter the native manager while it dispatches lifecycle signals.
        self.refresh_timer.start(0)

    def install_folder(self):
        self.install(QFileDialog.getExistingDirectory(self, tr('Dossier de l’extension Chromium')))

    def install_zip(self):
        path, _ = QFileDialog.getOpenFileName(self, tr('Extension Chromium'), '', tr('Archive ZIP (*.zip)'))
        self.install(path)

    def install(self, path):
        if not path:
            return
        try:
            name, permissions = inspect_webextension(path)
        except (OSError, ValueError, TypeError, UnicodeError, zipfile.BadZipFile) as error:
            self.status.setText(str(error))
            return
        question = QMessageBox(self)
        question.setWindowTitle(tr('Installer cette extension ?'))
        question.setTextFormat(Qt.TextFormat.PlainText)
        question.setText(name + tr('\nPermissions déclarées :\n') +
                         (', '.join(permissions) or tr('Aucune permission explicite')) +
                         tr('\nUne extension peut lire ou modifier les sites autorisés. Elle sera installée désactivée.'))
        question.setStandardButtons(QMessageBox.StandardButton.Yes | QMessageBox.StandardButton.No)
        question.setDefaultButton(QMessageBox.StandardButton.No)
        if question.exec() != QMessageBox.StandardButton.Yes:
            return
        self.folder.setEnabled(False)
        self.archive.setEnabled(False)
        self.status.setText(tr('Installation en cours…'))
        self.manager.installExtension(path)

    def toggle_selected(self):
        extension = self.selected()
        if extension:
            enabled = not extension.isEnabled()
            marker = Path(self.browser.profile_dir) / ('extension-activation-' + extension.id() + '.pending')
            if enabled and marker.exists():
                self.status.setText(tr('La précédente activation de cette extension ne s’est pas terminée. '
                                    'Activation bloquée pour éviter un nouveau crash. Vous pouvez la désinstaller.'))
                return
            if enabled:
                try:
                    marker.write_text('Native extension activation in progress', encoding='utf-8')
                except OSError as error:
                    self.status.setText(tr('Impossible de préparer le diagnostic : ') + str(error))
                    return
            self.toggle.setEnabled(False)
            self.remove.setEnabled(False)
            self.popup.setEnabled(False)
            record_extension_event(self.browser.data_dir,
                                   'before setExtensionEnabled enabled=' + str(enabled))
            self.manager.setExtensionEnabled(extension, enabled)
            record_extension_event(self.browser.data_dir, 'after setExtensionEnabled')
            if enabled:
                marker.unlink(missing_ok=True)
            self.refresh_timer.start(0)

    def open_popup(self):
        extension = self.selected()
        if extension and extension.isEnabled() and not extension.actionPopupUrl().isEmpty():
            self.browser.add_tab(extension.actionPopupUrl().toString(), True)
            self.accept()

    def remove_selected(self):
        extension = self.selected()
        if extension and QMessageBox.question(self, tr('Désinstaller'),
                tr('Supprimer cette extension du profil ?'),
                QMessageBox.StandardButton.Yes | QMessageBox.StandardButton.No,
                QMessageBox.StandardButton.No) == QMessageBox.StandardButton.Yes:
            self.manager.uninstallExtension(extension)

    def done(self, result):
        self.refresh_timer.stop()
        # Window is reused, so keep its native-manager signal connections.
        super().done(result)
