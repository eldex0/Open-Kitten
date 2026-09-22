from ui.dialog_style import panel_layout
from core.i18n import tr
"""Categorized settings editor; no persistence before validation and Save."""
from PyQt6.QtWidgets import (
    QCheckBox, QComboBox, QDialog, QDialogButtonBox, QFileDialog,
    QFormLayout, QHBoxLayout, QLabel, QLineEdit, QMessageBox,
    QPushButton, QSpinBox, QTabWidget, QVBoxLayout, QWidget, QScrollArea,
)
from core.settings import DEFAULT_SETTINGS
from core.search_engines import ENGINES
from core.i18n import UI_LANGUAGES
from PyQt6.QtCore import QLocale


class SettingsDialog(QDialog):
    def __init__(self, browser, parent=None):
        super().__init__(parent)
        self.browser = browser
        self.controls = {}
        self.setWindowTitle(tr("Paramètres"))
        self.resize(900, 660)
        layout = panel_layout(self)
        self.tabs = QTabWidget()
        layout.addWidget(self.tabs)
        general = self.section(tr("Général"))
        self.choice(general, "search_engine", tr("Moteur de recherche"),
                    [(x, x) for x in ENGINES])
        self.choice(general, 'ui_language', tr('Langue de l’interface (redémarrage)'),
                    [(tr('Automatique (langue du PC)'), 'auto')] + [(name, code) for code, name in UI_LANGUAGES.items()])
        note = QLabel(tr('La langue de l’interface sera appliquée au prochain démarrage.'))
        note.setWordWrap(True)
        general.addRow(note)
        self.choice(general, "language", tr("Langue demandée aux sites"),
                    [(tr('Automatique (langue du PC)'), 'auto')] + self.website_languages())
        self.choice(general, "homepage", tr("Page d’accueil"),
                    [(tr("Nouvel onglet"), "newtab"), (tr("Adresse personnalisée"), "custom")])
        self.text(general, "custom_homepage", tr("Adresse d’accueil"), "https://example.com")
        self.check(general, "restore_session", tr("Restaurer les onglets au démarrage"))
        self.check(general, "autosave_session", tr("Enregistrer la session pendant la navigation"))
        general.addRow(QLabel(tr("La session est aussi enregistrée à la fermeture.")))
        self.check(general, "show_splash", tr("Afficher l’animation de lancement"))
        appearance = self.section(tr("Apparence"))
        self.choice(appearance, "theme", tr("Thème"), [(tr("Sombre"), "dark"), (tr("Clair"), "light")])
        self.choice(appearance, 'accent_color', tr('Couleur d’accent'),
                    [(tr('Bleu'), 'blue'), (tr('Vert'), 'green'), (tr('Violet'), 'purple'), (tr('Orange'), 'orange')])
        self.number(appearance, 'ui_font_size', tr('Taille du texte de l’interface'), 11, 18, ' px')
        self.check(appearance, 'show_home_button', tr('Afficher le bouton Accueil'))
        self.check(appearance, 'show_status_bar', tr('Afficher la barre d’état des liens'))
        self.check(appearance, "show_bookmark_bar", tr("Afficher la barre de favoris"))
        self.check(appearance, 'home_show_logo', tr('Afficher le chat sur l’accueil'))
        self.number(appearance, 'home_shortcut_count', tr('Nombre de raccourcis sur l’accueil'), 0, 16, '')
        self.check(appearance, 'home_history_suggestions', tr('Utiliser l’historique si aucun favori n’existe'))
        self.number(appearance, "default_zoom", tr("Zoom par défaut"), 50, 200, " %")
        performance = self.section(tr("Performances"))
        self.check(performance, "lazy_restore", tr("Charger les onglets restaurés seulement à leur ouverture"))
        self.check(performance, "dns_prefetch", tr("Anticiper la résolution des adresses DNS"))
        self.number(performance, "cache_size_mb", tr("Taille maximale du cache"), 0, 2048, tr(" Mo"))
        performance.addRow(QLabel(tr("0 Mo : gestion automatique par QtWebEngine.")))
        performance.addRow(QLabel(tr("Le chargement différé s’applique au prochain démarrage.")))
        privacy = self.section(tr("Confidentialité"))
        self.check(privacy, "remember_history", tr("Enregistrer l’historique de navigation"))
        self.check(privacy, "block_popups", tr("Bloquer les nouvelles fenêtres des sites"))
        self.check(privacy, "autoplay", tr("Autoriser la lecture automatique des médias"))
        for label, callback in [
            (tr("Effacer l’historique"), self.clear_history),
            (tr("Effacer les cookies (déconnexion des sites)"), self.clear_cookies),
            (tr("Vider le cache"), self.clear_cache),
        ]:
            button = QPushButton(label)
            button.clicked.connect(callback)
            privacy.addRow(button)
        extensions = self.section(tr('Extensions 2.0'))
        note = QLabel(tr('Activation : Menu → Extensions. Ces options ne changent pas les extensions activées. '
                      'Après modification, rechargez les sites concernés. Les exceptions incluent les sous-domaines.'))
        note.setWordWrap(True)
        extensions.addRow(note)
        self.check(extensions, 'ad_cosmetic', tr('Ad Blocker : masquer les emplacements publicitaires ciblés'))
        self.text(extensions, 'ad_blocker_exceptions', tr('Exceptions Ad Blocker'), 'example.com, autre.fr')
        self.check(extensions, 'privacy_links', tr('Privacy Guard : protéger les liens externes et retirer les pings'))
        self.text(extensions, 'privacy_guard_exceptions', tr('Exceptions Privacy Guard'), 'example.com, autre.fr')
        self.check(extensions, 'dark_respect_native', tr('Dark Mode : conserver les pages déjà sombres'))
        self.choice(extensions, 'dark_palette', tr('Palette Dark Mode'), [(tr('Neutre'), 'neutral'), (tr('Chaude'), 'warm')])
        self.text(extensions, 'dark_mode_exceptions', tr('Exceptions Dark Mode'), 'example.com, autre.fr')
        network = self.section(tr("Réseau"))
        self.text(network, "download_directory", tr("Dossier des téléchargements"), tr("Dossier par défaut"))
        browse = QPushButton(tr("Choisir un dossier…"))
        browse.clicked.connect(self.choose_download_directory)
        network.addRow(browse)
        self.check(network, "proxy_enabled", tr("Utiliser un proxy (après redémarrage)"))
        self.choice(network, "proxy_type", tr("Type"), [("HTTP", "HTTP"), ("SOCKS5", "SOCKS5")])
        self.text(network, "proxy_host", tr("Serveur"), "127.0.0.1")
        self.text(network, "proxy_port", tr("Port"), "8080")
        network.addRow(QLabel(tr("Les proxys avec authentification ne sont pas pris en charge.")))
        footer = QHBoxLayout()
        reset = QPushButton(tr("Rétablir les valeurs par défaut"))
        reset.clicked.connect(self.reset_form)
        footer.addWidget(reset)
        buttons = QDialogButtonBox(QDialogButtonBox.StandardButton.Save |
                                   QDialogButtonBox.StandardButton.Cancel)
        buttons.accepted.connect(self.save)
        buttons.button(QDialogButtonBox.StandardButton.Save).setText(tr('Enregistrer'))
        buttons.button(QDialogButtonBox.StandardButton.Cancel).setText(tr('Annuler'))
        buttons.rejected.connect(self.reject)
        footer.addWidget(buttons)
        layout.addLayout(footer)
        self.controls["homepage"].currentIndexChanged.connect(self.update_enabled)
        self.controls["proxy_enabled"].toggled.connect(self.update_enabled)
        self.update_enabled()

    @staticmethod
    def website_languages():
        entries = {}
        for locale in QLocale.matchingLocales(QLocale.Language.AnyLanguage,
                                              QLocale.Script.AnyScript, QLocale.Country.AnyCountry):
            code = locale.bcp47Name()
            if code != 'C':
                entries[code] = (locale.nativeLanguageName() or code) + ' — ' + code
        return [(label, code) for code, label in sorted(entries.items(), key=lambda item: item[1].casefold())]

    def section(self, title):
        page = QWidget()
        form = QFormLayout(page)
        form.setFieldGrowthPolicy(QFormLayout.FieldGrowthPolicy.ExpandingFieldsGrow)
        scroll = QScrollArea()
        scroll.setWidgetResizable(True)
        scroll.setWidget(page)
        self.tabs.addTab(scroll, title)
        return form

    def check(self, form, key, label):
        widget = QCheckBox(label)
        widget.setChecked(self.browser.settings.get(key, DEFAULT_SETTINGS[key]))
        self.controls[key] = widget
        form.addRow(widget)

    def choice(self, form, key, label, choices):
        widget = QComboBox()
        for text, value in choices:
            widget.addItem(text, value)
        value = self.browser.settings.get(key, DEFAULT_SETTINGS[key])
        if key == 'proxy_type' and value == 'HTTPS':
            value = 'HTTP'
        widget.setCurrentIndex(max(0, widget.findData(value)))
        if widget.findData(value) < 0:
            widget.addItem(str(value), value)
            widget.setCurrentIndex(widget.count() - 1)
        self.controls[key] = widget
        form.addRow(label, widget)

    def text(self, form, key, label, placeholder):
        widget = QLineEdit(self.browser.settings.get(key, DEFAULT_SETTINGS[key]))
        widget.setPlaceholderText(placeholder)
        self.controls[key] = widget
        form.addRow(label, widget)

    def number(self, form, key, label, minimum, maximum, suffix):
        widget = QSpinBox()
        widget.setRange(minimum, maximum)
        widget.setSuffix(suffix)
        widget.setValue(self.browser.settings.get(key, DEFAULT_SETTINGS[key]))
        self.controls[key] = widget
        form.addRow(label, widget)

    def update_enabled(self, *args):
        self.controls["custom_homepage"].setEnabled(self.controls["homepage"].currentData() == "custom")
        for key in ("proxy_type", "proxy_host", "proxy_port"):
            self.controls[key].setEnabled(self.controls["proxy_enabled"].isChecked())

    def choose_download_directory(self):
        directory = QFileDialog.getExistingDirectory(self, tr("Dossier des téléchargements"))
        if directory:
            self.controls["download_directory"].setText(directory)

    def confirmed(self, message):
        return QMessageBox.question(self, tr("Confirmer"), message,
                                    QMessageBox.StandardButton.Yes | QMessageBox.StandardButton.No,
                                    QMessageBox.StandardButton.No) == QMessageBox.StandardButton.Yes

    def clear_history(self):
        if self.confirmed(tr("Effacer définitivement l’historique de ce profil ?")):
            self.browser.privacy.clear_history()

    def clear_cookies(self):
        if self.confirmed(tr("Supprimer les cookies de ce profil ? Cela déconnecte les sites.")):
            self.browser.privacy.clear_cookies()
            QMessageBox.information(self, tr("Cookies"), tr("La suppression des cookies a été demandée."))

    def clear_cache(self):
        if self.confirmed(tr("Vider le cache de ce profil ?")):
            self.browser.privacy.clear_cache()
            QMessageBox.information(self, tr("Cache"), tr("Le nettoyage a été demandé et se poursuit en arrière-plan."))

    def reset_form(self):
        if not self.confirmed(tr("Rétablir les valeurs par défaut dans ce formulaire ? Cliquez ensuite sur Enregistrer pour les appliquer.")):
            return
        for key, widget in self.controls.items():
            value = DEFAULT_SETTINGS[key]
            if isinstance(widget, QCheckBox):
                widget.setChecked(value)
            elif isinstance(widget, QComboBox):
                widget.setCurrentIndex(widget.findData(value))
            elif isinstance(widget, QSpinBox):
                widget.setValue(value)
            else:
                widget.setText(value)
        self.update_enabled()

    def save(self):
        values = {}
        for key, widget in self.controls.items():
            if isinstance(widget, QCheckBox):
                value = widget.isChecked()
            elif isinstance(widget, QComboBox):
                value = widget.currentData()
            elif isinstance(widget, QSpinBox):
                value = widget.value()
            else:
                value = widget.text().strip()
            values[key] = value
        old = self.browser.settings.settings.copy()
        try:
            self.browser.settings.update(values)
        except (ValueError, OSError) as error:
            QMessageBox.warning(self, tr("Paramètres non enregistrés"), str(error))
            return
        self.browser.apply_settings()
        if old.get('ui_language', 'fr') != values.get('ui_language'):
            QMessageBox.information(self, tr('Redémarrage nécessaire'),
                                    tr('Redémarrez Open Kitten pour appliquer la langue de l’interface.'))
        extension_keys = ('ad_cosmetic', 'privacy_links', 'dark_respect_native', 'dark_palette',
                          'ad_blocker_exceptions', 'privacy_guard_exceptions', 'dark_mode_exceptions')
        if any(old.get(key) != values.get(key) for key in extension_keys):
            self.browser.extension_manager.apply_to_profile(self.browser.profile)
            QMessageBox.information(self, tr('Extensions mises à jour'),
                                    tr('Les réglages sont enregistrés. Rechargez les sites déjà ouverts pour appliquer les scripts.'))
        if any(old.get(key) != values[key] for key in values if key.startswith("proxy_")):
            QMessageBox.information(self, tr("Redémarrage nécessaire"), tr("Redémarrez le navigateur pour appliquer le proxy."))
        self.accept()
