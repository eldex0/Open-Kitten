"""Qt standard controls and bundled Qt translations, kept alive by QApplication."""
from PyQt6.QtCore import QLibraryInfo, QTranslator
from core.i18n import tr


class StandardButtonsTranslator(QTranslator):
    def isEmpty(self):
        return False

    def translate(self, context, sourceText, disambiguation=None, n=-1):
        labels = {'Yes': 'Oui', 'No': 'Non', 'OK': 'OK', 'Cancel': 'Annuler',
                  'Save': 'Enregistrer', 'Close': 'Fermer', 'Open': 'Ouvrir'}
        source = sourceText.replace('&', '')
        if context in ('QDialogButtonBox', 'QMessageBox', 'QPlatformTheme') and source in labels:
            return tr(labels[source])
        return ''


def install_qt_translations(app, language):
    directory = QLibraryInfo.path(QLibraryInfo.LibraryPath.TranslationsPath)
    translators = []
    variants = {'zh': ('zh_CN', 'zh'), 'pt': ('pt_PT', 'pt_BR', 'pt')}.get(language, (language,))
    for name in ('qtbase_', 'qt_'):
        translator = QTranslator(app)
        if any(translator.load(name + variant, directory) for variant in variants):
            app.installTranslator(translator)
            translators.append(translator)
    buttons = StandardButtonsTranslator(app)
    app.installTranslator(buttons)
    translators.append(buttons)
    app.open_kitten_translators = translators
