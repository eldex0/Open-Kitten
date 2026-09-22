from ui.dialog_style import panel_layout
from core.i18n import tr
from PyQt6.QtWidgets import (
    QDialog,
    QInputDialog,
    QLabel,
    QListWidget,
    QMessageBox,
    QPushButton,
    QVBoxLayout,
)


class ProfilesDialog(QDialog):

    def __init__(
        self,
        browser,
        parent=None
    ):
        super().__init__(parent)

        self.browser = browser

        self.setWindowTitle(
            tr("Profils")
        )

        self.resize(
            400,
            450
        )

        layout = panel_layout(self)

        layout.addWidget(
            QLabel(
                tr("Profil actuel : ")
                + browser.profile_name
            )
        )

        self.list = QListWidget()

        layout.addWidget(
            self.list
        )

        new_button = QPushButton(
            tr("Nouveau profil")
        )

        delete_button = QPushButton(
            tr("Supprimer")
        )

        switch_button = QPushButton(
            tr("Utiliser ce profil")
        )

        layout.addWidget(
            new_button
        )

        layout.addWidget(
            delete_button
        )

        layout.addWidget(
            switch_button
        )

        new_button.clicked.connect(
            self.create_profile
        )

        delete_button.clicked.connect(
            self.delete_profile
        )

        switch_button.clicked.connect(
            self.switch_profile
        )

        self.refresh()

    def refresh(self):

        self.list.clear()

        profiles = self.browser.profile_manager.list_profiles()

        for profile in profiles:
            self.list.addItem(profile)

    def create_profile(self):

        name, ok = QInputDialog.getText(
            self,
            tr("Nouveau profil"),
            tr("Nom du profil :")
        )

        if not ok or not name.strip():
            return

        self.browser.profile_manager.create_profile(
            name
        )

        self.refresh()

    def delete_profile(self):

        item = self.list.currentItem()

        if not item:
            return

        name = item.text()

        if name == self.browser.profile_name:
            QMessageBox.warning(
                self,
                tr("Profil utilisé"),
                tr("Impossible de supprimer le profil actuel.")
            )
            return

        if name == "Default":
            QMessageBox.warning(
                self,
                tr("Profil protégé"),
                tr("Le profil Default ne peut pas être supprimé.")
            )
            return

        self.browser.profile_manager.delete_profile(
            name
        )

        self.refresh()

    def switch_profile(self):

        item = self.list.currentItem()

        if not item:
            return

        name = item.text()

        if name == self.browser.profile_name:
            self.accept()
            return

        QMessageBox.information(
            self,
            tr("Redémarrage nécessaire"),
            tr("Le nouveau profil sera utilisé au prochain démarrage.")
        )

        self.browser.settings.set(
            "default_profile",
            name
        )

        self.accept()
