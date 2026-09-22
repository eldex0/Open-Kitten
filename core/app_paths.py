"""Separate bundled resources from writable user data."""
import os
import sys
from pathlib import Path

APP_NAME = "Open Kitten"
VERSION = "1.0.0"
RESOURCE_DIR = Path(getattr(sys, '_MEIPASS', Path(__file__).resolve().parent.parent))


def data_directory():
    # Preserve the existing development installation. Distributed builds use
    # the Windows user's directory, never Program Files or the bundle.
    if not getattr(sys, 'frozen', False):
        return RESOURCE_DIR / 'data'
    return Path(os.environ.get('LOCALAPPDATA', Path.home() / 'AppData' / 'Local')) / 'MiniBrowser' / 'data'
