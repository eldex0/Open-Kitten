"""Bundled UI translations and system-language resolution.

No web service receives text or browsing data. Additional catalogs can be added
here without changing storage keys, browser URLs or extension identifiers.
"""
UI_LANGUAGES = {
    'fr': 'Français', 'en': 'English', 'de': 'Deutsch', 'es': 'Español',
    'it': 'Italiano', 'pt': 'Português', 'nl': 'Nederlands',
    'ar': 'العربية', 'zh': '中文', 'ja': '日本語',
}
_language = 'fr'
_system_languages = ['en']


def set_system_languages(languages):
    global _system_languages
    _system_languages = list(languages) or ['en']


def resolve_language(choice='auto', system_languages=None):
    if choice != 'auto' and choice in UI_LANGUAGES:
        return choice
    for tag in (_system_languages if system_languages is None else system_languages):
        code = str(tag).replace('_', '-').split('-')[0].lower()
        if code in UI_LANGUAGES:
            return code
    return 'en'


def website_language(choice):
    if choice != 'auto':
        return choice
    import re
    return next((tag.replace('_', '-') for tag in _system_languages
                 if re.fullmatch(r'[a-zA-Z]{2,3}(?:[-_][a-zA-Z0-9]{2,8})*', tag)), 'en')
EN = {
    'Automatique (langue du PC)': 'Automatic (PC language)',
    'Favoris': 'Bookmarks', 'Historique': 'History', 'Téléchargements': 'Downloads',
    'Nouvelle fenêtre privée': 'New private window', 'Profils': 'Profiles',
    'Extensions': 'Extensions', 'Paramètres': 'Settings', 'Plein écran (F11)': 'Full screen (F11)',
    'À propos': 'About', 'Fermer': 'Close', 'Supprimer': 'Delete', 'Tout effacer': 'Clear all',
    'Rechercher': 'Search', 'Annuler': 'Cancel', 'Enregistrer': 'Save',
    'Général': 'General', 'Apparence': 'Appearance', 'Performances': 'Performance',
    'Confidentialité': 'Privacy', 'Réseau': 'Network', 'Extensions 2.0': 'Extensions 2.0',
    'Langue de l’interface (redémarrage)': 'Interface language (restart required)',
    'Langue demandée aux sites': 'Preferred website language',
    'La langue de l’interface sera appliquée au prochain démarrage.':
        'Language changes apply after restart.',
    'Moteur de recherche': 'Search engine', 'Page d’accueil': 'Home page',
    'Nouvel onglet': 'New tab', 'Adresse personnalisée': 'Custom address',
    'Adresse d’accueil': 'Home address', 'Thème': 'Theme', 'Sombre': 'Dark', 'Clair': 'Light',
    'Couleur d’accent': 'Accent color', 'Bleu': 'Blue', 'Vert': 'Green', 'Violet': 'Purple', 'Orange': 'Orange',
    'Taille du texte de l’interface': 'Interface text size',
    'Afficher le bouton Accueil': 'Show Home button', 'Afficher la barre d’état des liens': 'Show link status bar',
    'Afficher la barre de favoris': 'Show bookmarks bar', 'Afficher le chat sur l’accueil': 'Show the cat on the start page',
    'Nombre de raccourcis sur l’accueil': 'Start page shortcut count',
    'Utiliser l’historique si aucun favori n’existe': 'Use history when there are no bookmarks',
    'Zoom par défaut': 'Default zoom', 'Restaurer les onglets au démarrage': 'Restore tabs on startup',
    'Enregistrer la session pendant la navigation': 'Save session while browsing',
    'La session est aussi enregistrée à la fermeture.': 'The session is also saved when closing.',
    'Afficher l’animation de lancement': 'Show startup animation',
    'Charger les onglets restaurés seulement à leur ouverture': 'Load restored tabs only when selected',
    'Anticiper la résolution des adresses DNS': 'Prefetch DNS addresses',
    'Taille maximale du cache': 'Maximum cache size',
    '0 Mo : gestion automatique par QtWebEngine.': '0 MB: managed automatically by QtWebEngine.',
    'Le chargement différé s’applique au prochain démarrage.': 'Lazy loading applies at the next startup.',
    'Enregistrer l’historique de navigation': 'Record browsing history',
    'Bloquer les nouvelles fenêtres des sites': 'Block website pop-up windows',
    'Autoriser la lecture automatique des médias': 'Allow media autoplay',
    'Effacer l’historique': 'Clear history', 'Effacer les cookies (déconnexion des sites)': 'Clear cookies (sign out of websites)',
    'Vider le cache': 'Clear cache', 'Dossier des téléchargements': 'Download folder',
    'Dossier par défaut': 'Default folder', 'Choisir un dossier…': 'Choose a folder…',
    'Utiliser un proxy (après redémarrage)': 'Use a proxy (restart required)',
    'Type': 'Type', 'Serveur': 'Server', 'Port': 'Port',
    'Les proxys avec authentification ne sont pas pris en charge.': 'Authenticated proxies are not supported.',
    'Rétablir les valeurs par défaut': 'Restore defaults', 'Confirmer': 'Confirm',
    'Paramètres non enregistrés': 'Settings not saved', 'Redémarrage nécessaire': 'Restart required',
    'Palette Dark Mode': 'Dark Mode palette', 'Neutre': 'Neutral', 'Chaude': 'Warm',
    'Exceptions Ad Blocker': 'Ad Blocker exceptions', 'Exceptions Privacy Guard': 'Privacy Guard exceptions',
    'Exceptions Dark Mode': 'Dark Mode exceptions',
    'Ad Blocker : masquer les emplacements publicitaires ciblés': 'Ad Blocker: hide targeted ad slots',
    'Privacy Guard : protéger les liens externes et retirer les pings': 'Privacy Guard: protect external links and remove pings',
    'Dark Mode : conserver les pages déjà sombres': 'Dark Mode: preserve already dark pages',
    'Rechercher dans l\'historique...': 'Search history…', 'Rechercher dans les favoris...': 'Search bookmarks…',
    'Rechercher dans la page': 'Find in page', 'Texte à rechercher...': 'Find text…',
    'Nouveau profil': 'New profile', 'Utiliser ce profil': 'Use this profile', 'Nom du profil :': 'Profile name:',
    'Ouvrir': 'Open', 'Dossier': 'Folder', 'Aucun téléchargement': 'No downloads',
    'Précédent': 'Back', 'Suivant': 'Forward', 'Actualiser': 'Reload', 'Accueil': 'Home',
    'Rechercher ou saisir une adresse...': 'Search or enter an address…',
    'Couper le son': 'Mute tab', 'Rétablir le son': 'Unmute tab', 'Copier l’adresse': 'Copy address',
    'Fermer les autres': 'Close other tabs', 'Restaurer un onglet': 'Reopen closed tab',
    "Dupliquer l'onglet": 'Duplicate tab', "Épingler l'onglet": 'Pin tab', "Retirer l'épingle": 'Unpin tab',
    'Activer': 'Enable', 'Désactiver': 'Disable', 'Installer depuis un dossier…': 'Install from folder…',
    'Suivez vos fichiers en cours et retrouvez-les rapidement.': 'Track your files and find them quickly.',
    'Préparation de votre espace de navigation…': 'Preparing your browser…',
    'Aucun résultat': 'No results', 'Double-cliquez pour ouvrir un élément.': 'Double-click an item to open it.',
    'Effacer définitivement tout l’historique ?': 'Permanently clear all history?',
}
KEYS = ('Favoris', 'Historique', 'Téléchargements', 'Nouvelle fenêtre privée', 'Profils',
        'Extensions', 'Paramètres', 'À propos', 'Fermer', 'Supprimer', 'Rechercher',
        'Général', 'Apparence', 'Performances', 'Confidentialité', 'Réseau', 'Nouvel onglet',
        'Sombre', 'Clair', 'Enregistrer', 'Annuler')
ROWS = {
 'de': 'Lesezeichen|Verlauf|Downloads|Neues privates Fenster|Profile|Erweiterungen|Einstellungen|Über|Schließen|Löschen|Suchen|Allgemein|Darstellung|Leistung|Datenschutz|Netzwerk|Neuer Tab|Dunkel|Hell|Speichern|Abbrechen',
 'es': 'Marcadores|Historial|Descargas|Nueva ventana privada|Perfiles|Extensiones|Configuración|Acerca de|Cerrar|Eliminar|Buscar|General|Apariencia|Rendimiento|Privacidad|Red|Nueva pestaña|Oscuro|Claro|Guardar|Cancelar',
 'it': 'Preferiti|Cronologia|Download|Nuova finestra privata|Profili|Estensioni|Impostazioni|Informazioni|Chiudi|Elimina|Cerca|Generale|Aspetto|Prestazioni|Privacy|Rete|Nuova scheda|Scuro|Chiaro|Salva|Annulla',
 'pt': 'Favoritos|Histórico|Transferências|Nova janela privada|Perfis|Extensões|Definições|Sobre|Fechar|Eliminar|Pesquisar|Geral|Aparência|Desempenho|Privacidade|Rede|Novo separador|Escuro|Claro|Guardar|Cancelar',
 'nl': 'Bladwijzers|Geschiedenis|Downloads|Nieuw privévenster|Profielen|Extensies|Instellingen|Over|Sluiten|Verwijderen|Zoeken|Algemeen|Uiterlijk|Prestaties|Privacy|Netwerk|Nieuw tabblad|Donker|Licht|Opslaan|Annuleren',
 'ar': 'الإشارات المرجعية|السجل|التنزيلات|نافذة خاصة جديدة|الملفات الشخصية|الإضافات|الإعدادات|حول|إغلاق|حذف|بحث|عام|المظهر|الأداء|الخصوصية|الشبكة|علامة تبويب جديدة|داكن|فاتح|حفظ|إلغاء',
 'zh': '书签|历史记录|下载|新建隐私窗口|个人资料|扩展|设置|关于|关闭|删除|搜索|常规|外观|性能|隐私|网络|新标签页|深色|浅色|保存|取消',
 'ja': 'ブックマーク|履歴|ダウンロード|新しいプライベートウィンドウ|プロファイル|拡張機能|設定|このアプリについて|閉じる|削除|検索|一般|外観|パフォーマンス|プライバシー|ネットワーク|新しいタブ|ダーク|ライト|保存|キャンセル',
}
CATALOGS = {code: dict(zip(KEYS, row.split('|'))) for code, row in ROWS.items()}
CATALOGS['en'] = EN
from core.translation_messages import MESSAGES
from core.translation_errors import MESSAGES as ERROR_MESSAGES
MESSAGES = {**MESSAGES, **ERROR_MESSAGES}
for source, translated in MESSAGES.items():
    EN[source] = translated['en']
    for code, value in translated.items():
        CATALOGS[code][source] = value
from core.translations import TRANSLATIONS
for code in CATALOGS:
    if code != 'en':
        CATALOGS[code].update({source: TRANSLATIONS[english][code]
                               for source, english in EN.items() if english in TRANSLATIONS})

# Localized file-filter captions preserve the actual wildcard syntax.
for source, values in {
    'Archive ZIP (*.zip)': ('ZIP archive (*.zip)', 'ZIP-Archiv (*.zip)', 'Archivo ZIP (*.zip)',
                          'Archivio ZIP (*.zip)', 'Arquivo ZIP (*.zip)', 'ZIP-archief (*.zip)',
                          'أرشيف ZIP (*.zip)', 'ZIP 压缩包 (*.zip)', 'ZIP アーカイブ (*.zip)'),
    ' Mo': (' MB', ' MB', ' MB', ' MB', ' MB', ' MB', ' MB', ' MB', ' MB'),
}.items():
    EN[source] = values[0]
    for code, value in zip(('en', 'de', 'es', 'it', 'pt', 'nl', 'ar', 'zh', 'ja'), values):
        CATALOGS[code][source] = value


def set_language(language):
    global _language
    _language = resolve_language(language)


def tr(source, language=None):
    code = resolve_language(language) if language else _language
    if code == 'fr':
        return source
    return CATALOGS.get(code, {}).get(source, EN.get(source, source))
