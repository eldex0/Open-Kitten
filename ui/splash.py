from core.i18n import tr
from PyQt6.QtCore import Qt, QTimer, pyqtSignal
from core.app_paths import VERSION, RESOURCE_DIR
from PyQt6.QtGui import QIcon
from PyQt6.QtWidgets import (
    QDialog,
    QLabel,
    QProgressBar,
    QVBoxLayout,
)


class StartupSplash(QDialog):

    finished = pyqtSignal()

    def __init__(self):
        super().__init__()

        self.setWindowFlags(
            Qt.WindowType.FramelessWindowHint
            | Qt.WindowType.SplashScreen
        )
        self.setFixedSize(440, 260)

        layout = QVBoxLayout(self)
        layout.setContentsMargins(36, 36, 36, 36)
        layout.setSpacing(14)

        logo = QLabel()
        logo.setPixmap(QIcon(str(RESOURCE_DIR / 'assets' / 'icon.svg')).pixmap(72, 72))
        logo.setAlignment(Qt.AlignmentFlag.AlignCenter)

        title = QLabel("Open Kitten " + VERSION)
        title.setAlignment(Qt.AlignmentFlag.AlignCenter)
        title.setStyleSheet("font-size: 25px; font-weight: 700;")

        self.status = QLabel(tr("Préparation de votre espace de navigation…"))
        self.status.setAlignment(Qt.AlignmentFlag.AlignCenter)
        self.status.setStyleSheet("color: #a8abb5;")

        self.progress = QProgressBar()
        self.progress.setRange(0, 100)
        self.progress.setTextVisible(False)
        self.progress.setStyleSheet("""
            QProgressBar { border: 0; border-radius: 5px; background: #30323d; }
            QProgressBar::chunk { border-radius: 5px; background: #62ccf3; }
        """)

        layout.addStretch()
        layout.addWidget(logo)
        layout.addWidget(title)
        layout.addWidget(self.status)
        layout.addWidget(self.progress)
        layout.addStretch()

        self.setStyleSheet("background: #102036; color: #eff8ff;")

        self.timer = QTimer(self)
        self.timer.timeout.connect(self.advance)

    def start(self):
        self.progress.setValue(0)
        self.timer.start(18)

    def advance(self):
        value = self.progress.value() + 4
        self.progress.setValue(value)

        if value >= 100:
            self.timer.stop()
            self.finished.emit()
