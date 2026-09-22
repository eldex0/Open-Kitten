"""Local activation diagnostics; no page content, credentials or URLs."""
import faulthandler
import sys
from datetime import datetime, timezone
from pathlib import Path

_stream = None


def record_extension_event(data_dir, event):
    global _stream
    try:
        if _stream is None:
            folder = Path(data_dir) / 'diagnostics'
            folder.mkdir(parents=True, exist_ok=True)
            # One file per process, overwritten on its first native-extension action.
            _stream = (folder / 'extensions-crash.log').open('w', encoding='utf-8')
            faulthandler.enable(file=_stream, all_threads=True)
            from PyQt6.QtCore import QT_VERSION_STR, PYQT_VERSION_STR
            _stream.write(f'Python {sys.version.split()[0]} / Qt {QT_VERSION_STR} / PyQt {PYQT_VERSION_STR}\n')
        _stream.write(datetime.now(timezone.utc).isoformat() + ' ' + event + '\n')
        _stream.flush()
    except (OSError, RuntimeError):
        # Diagnostics must never prevent navigation.
        pass
