"""Compact shared palette and widget styles; no navigation logic."""
from core.appearance import accent_color
PALETTES = {
    'dark': dict(bg='#17191e', panel='#21242b', raised='#2c3039', text='#edf0f5',
                 muted='#a3a9b7', line='#363b46', accent='#92aaff'),
    'light': dict(bg='#f6f7f9', panel='#ffffff', raised='#e9ecf2', text='#242831',
                  muted='#646b79', line='#dce0e7', accent='#315bdb'),
}


def stylesheet(theme, settings=None):
    p = PALETTES.get(theme, PALETTES['dark']).copy()
    settings = settings if settings is not None else {'theme': theme}
    p['accent'] = accent_color(settings)
    p['font_size'] = settings.get('ui_font_size', 13)
    return """
        QWidget { background: %(bg)s; color: %(text)s; font-family: 'Segoe UI'; font-size: %(font_size)spx; }
        #navigationBar { background: %(panel)s; border-bottom: 1px solid %(line)s; }
        #bookmarkBar { background: %(panel)s; border-bottom: 1px solid %(line)s; }
        #securityLabel { background: transparent; min-width: 24px; }
        QLineEdit { background: %(bg)s; border: 1px solid %(line)s; border-radius: 7px;
                    padding: 7px 10px; min-height: 22px; selection-background-color: %(accent)s; }
        QLineEdit:focus { border-color: %(accent)s; }
        QPushButton, QToolButton { background: %(panel)s; border: 1px solid %(line)s;
                                  border-radius: 5px; padding: 6px 10px; }
        QPushButton:hover, QToolButton:hover { background: %(raised)s; }
        QPushButton:disabled, QToolButton:disabled { color: %(muted)s; }
        QTabWidget::pane { border: 0; }
        QTabBar::tab { background: %(bg)s; color: %(muted)s; padding: 8px 14px;
                       margin: 3px 2px 0 2px; border-bottom: 2px solid transparent; }
        QTabBar::tab:selected { background: %(panel)s; color: %(text)s; border-bottom-color: %(accent)s; }
        QTabBar::tab:hover { background: %(raised)s; }
        QMenu { background: %(panel)s; border: 1px solid %(line)s; padding: 5px; }
        QMenu::item { padding: 8px 24px; }
        QMenu::item:selected { background: %(raised)s; border-radius: 4px; }
        QMenu::separator { height: 1px; background: %(line)s; margin: 5px 8px; }
        QComboBox, QSpinBox { background: %(panel)s; border: 1px solid %(line)s; border-radius: 5px; padding: 5px; }
        QComboBox QAbstractItemView { background: %(panel)s; selection-background-color: %(raised)s; }
        QListWidget, QTreeWidget { background: %(panel)s; border: 1px solid %(line)s; }
        QListWidget::item { padding: 10px 12px; border-bottom: 1px solid %(line)s; }
        #panelTitle { font-size: 24px; font-weight: 600; background: transparent; }
        #panelHint { color: %(muted)s; background: transparent; }
        QListWidget::item:selected, QTreeWidget::item:selected { background: %(raised)s; }
        QCheckBox { spacing: 8px; padding: 4px 0; }
        QGroupBox { border: 1px solid %(line)s; border-radius: 6px; margin-top: 14px; padding: 12px; }
        QGroupBox::title { subcontrol-origin: margin; left: 12px; }
        QScrollArea { border: 0; }
        QScrollBar:vertical { background: %(bg)s; width: 10px; }
        QScrollBar::handle:vertical { background: %(line)s; border-radius: 4px; min-height: 24px; }
        QScrollBar::add-line:vertical, QScrollBar::sub-line:vertical { height: 0; }
        QProgressBar { background: %(raised)s; border: 0; border-radius: 3px; height: 6px; }
        QProgressBar::chunk { background: %(accent)s; border-radius: 3px; }
        #pageLoadProgress { background: %(panel)s; border: 0; border-radius: 0; min-height: 2px; max-height: 2px; }
        #pageLoadProgress::chunk { background: %(accent)s; border-radius: 0; }
        #downloadsTitle { font-size: 22px; font-weight: 600; }
        #downloadsSubtitle, #downloadsCount { color: %(muted)s; }
        #downloadItem { background: %(panel)s; border: 1px solid %(line)s; border-radius: 7px; }
        QStatusBar { background: %(panel)s; color: %(muted)s; border-top: 1px solid %(line)s; }
        QStatusBar::item { border: 0; }
    """ % p
