"""Original monochrome vector icons, independent of emoji fonts."""
from PyQt6.QtCore import QByteArray, Qt
from PyQt6.QtGui import QIcon, QPixmap, QPainter
from PyQt6.QtSvg import QSvgRenderer
from functools import lru_cache

PATHS = {
    'stop': '<path d="m6 6 12 12M18 6 6 18"/>',
    'back': '<path d="m14 5-7 7 7 7M7 12h14"/>',
    'forward': '<path d="m10 5 7 7-7 7M3 12h14"/>',
    'reload': '<path d="M20 7v5h-5M20 12a8 8 0 1 0-2 6"/>',
    'home': '<path d="m3 11 9-8 9 8M5 10v11h5v-7h4v7h5V10"/>',
    'private': '<path d="M3 11h18M6 11l2-7h8l2 7M10 17h4"/><circle cx="7" cy="17" r="3"/><circle cx="17" cy="17" r="3"/>',
    'shield': '<path d="m12 3 8 3v6c0 5-8 9-8 9S4 17 4 12V6Z"/>',
    'star': '<path d="m12 3 2.8 5.7 6.2.9-4.5 4.4 1.1 6.2-5.6-3-5.6 3 1.1-6.2L3 9.6l6.2-.9Z"/>',
    'menu': '<path d="M4 6h16M4 12h16M4 18h16"/>',
    'plus': '<path d="M12 5v14M5 12h14"/>',
    'lock': '<rect x="5" y="10" width="14" height="11" rx="2"/><path d="M8 10V7a4 4 0 0 1 8 0v3M12 14v3"/>',
    'info': '<circle cx="12" cy="12" r="9"/><path d="M12 11v6M12 7v.2"/>',
    'warning': '<path d="m12 3 10 18H2ZM12 9v5M12 17v.2"/>',
}


@lru_cache(maxsize=64)
def icon(name, color='#bbc7d5', filled=False):
    fill = color if filled else 'none'
    svg = f'<svg xmlns="http://www.w3.org/2000/svg" viewBox="0 0 24 24" fill="{fill}" stroke="{color}" stroke-width="1.7" stroke-linecap="round" stroke-linejoin="round">{PATHS[name]}</svg>'
    pixmap = QPixmap(48, 48)
    pixmap.fill(Qt.GlobalColor.transparent)
    painter = QPainter(pixmap)
    QSvgRenderer(QByteArray(svg.encode())).render(painter)
    painter.end()
    return QIcon(pixmap)
