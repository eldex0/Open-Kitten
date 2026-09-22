"""Shared spacing for application panels, independent from website styling."""
from PyQt6.QtWidgets import QVBoxLayout


def panel_layout(dialog):
    dialog.setMinimumWidth(560)
    layout = QVBoxLayout(dialog)
    layout.setContentsMargins(24, 22, 24, 20)
    layout.setSpacing(14)
    return layout
