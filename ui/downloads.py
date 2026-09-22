from core.i18n import tr
import os

from PyQt6.QtCore import Qt
from PyQt6.QtWidgets import (
    QDialog,
    QFrame,
    QHBoxLayout,
    QLabel,
    QPushButton,
    QProgressBar,
    QScrollArea,
    QVBoxLayout,
    QWidget,
)


class DownloadItem(QWidget):

    def __init__(
        self,
        download,
        parent=None
    ):
        super().__init__(parent)

        self.download = download

        self.setObjectName("downloadItem")

        self.layout = QVBoxLayout(self)
        self.layout.setContentsMargins(14, 12, 14, 12)
        self.layout.setSpacing(8)

        self.name_label = QLabel(
            download.suggestedFileName()
            or tr("Téléchargement")
        )

        self.progress = QProgressBar()

        self.progress.setRange(
            0,
            100
        )

        self.status = QLabel(
            tr("Téléchargement...")
        )

        buttons = QHBoxLayout()

        self.open_button = QPushButton(
            tr("Ouvrir")
        )
        self.open_button.setEnabled(False)

        self.folder_button = QPushButton(
            tr("Dossier")
        )

        buttons.addWidget(
            self.open_button
        )

        buttons.addWidget(
            self.folder_button
        )

        self.layout.addWidget(
            self.name_label
        )

        self.layout.addWidget(
            self.progress
        )

        self.layout.addWidget(
            self.status
        )

        self.layout.addLayout(
            buttons
        )

        self.open_button.clicked.connect(
            self.open_file
        )

        self.folder_button.clicked.connect(
            self.open_folder
        )

        download.receivedBytesChanged.connect(
            self.update_progress
        )

        download.totalBytesChanged.connect(
            self.update_progress
        )

        download.stateChanged.connect(
            self.state_changed
        )

    def update_progress(self):

        total = self.download.totalBytes()
        received = self.download.receivedBytes()

        if total > 0:
            percent = int(
                received * 100 / total
            )

            self.progress.setValue(
                percent
            )

    def state_changed(self, state):

        state_name = str(state)

        if "DownloadCompleted" in state_name:
            self.status.setText(
                tr("Terminé · prêt à ouvrir")
            )
            self.progress.setValue(100)
            self.open_button.setEnabled(True)

        elif "DownloadCancelled" in state_name:
            self.status.setText(
                tr("Annulé")
            )

        elif "DownloadInterrupted" in state_name:
            self.status.setText(
                tr("Interrompu")
            )

    def open_file(self):

        path = self.download.downloadDirectory()
        filename = self.download.downloadFileName()

        full_path = os.path.join(
            path,
            filename
        )

        if os.path.exists(full_path):
            os.startfile(full_path)

    def open_folder(self):

        path = self.download.downloadDirectory()

        if os.path.exists(path):
            os.startfile(path)


class DownloadsDialog(QDialog):

    def __init__(
        self,
        parent=None
    ):
        super().__init__(parent)

        self.setWindowTitle(
            "Open Kitten — " + tr('Téléchargements')
        )

        self.resize(
            600,
            500
        )

        self.main_layout = QVBoxLayout(
            self
        )
        self.main_layout.setContentsMargins(18, 18, 18, 18)
        self.main_layout.setSpacing(12)

        title = QLabel(tr("Téléchargements"))
        title.setObjectName("downloadsTitle")

        subtitle = QLabel(
            tr("Suivez vos fichiers en cours et retrouvez-les rapidement.")
        )
        subtitle.setObjectName("downloadsSubtitle")

        self.count_label = QLabel(tr("Aucun téléchargement"))
        self.count_label.setObjectName("downloadsCount")

        self.main_layout.addWidget(title)
        self.main_layout.addWidget(subtitle)
        self.main_layout.addWidget(self.count_label)

        self.scroll = QScrollArea()

        self.scroll.setWidgetResizable(
            True
        )

        self.container = QWidget()

        self.container_layout = QVBoxLayout(
            self.container
        )
        self.container_layout.setContentsMargins(4, 4, 4, 4)
        self.container_layout.setSpacing(10)

        self.scroll.setWidget(
            self.container
        )

        self.main_layout.addWidget(
            self.scroll
        )

        self.download_count = 0

    def add_download(self, download):

        item = DownloadItem(
            download
        )

        self.container_layout.addWidget(
            item
        )

        self.download_count += 1

        self.count_label.setText(
            tr('Téléchargements : {count}').format(count=self.download_count)
        )
