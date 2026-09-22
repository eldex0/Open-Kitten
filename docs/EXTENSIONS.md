# Installer une extension

## Extensions Chrome / Chromium (support natif conditionnel)

Dans Extensions, ouvrir « Extensions Chrome / Chromium… ». Cette fenêtre utilise
QWebEngineExtensionManager, disponible à partir de Qt 6.10, si le binding PyQt l'expose.
Pour préparer cet environnement : `python -m pip install -r requirements-webextensions.txt`.
Installation : dossier décompressé ou ZIP avec manifest.json à la racine, Manifest V3.
Les paquets CRX/XPI, Manifest V2 et spécifiques Firefox sont refusés. Pour une extension
Firefox, utiliser la version Chromium de son auteur ; aucune conversion automatique
des API browser.* ou des arrière-plans Firefox n'est fournie.
L'installation est propre au profil courant, sans accès en navigation privée.
Activer manuellement après installation et après redémarrage (comportement Qt).
« Ouvrir l’extension » ouvre son actionPopupUrl dans un onglet du même profil ;
les extensions sans panneau popup n'ont pas cette action.
La compatibilité dépend des API de l'extension. Pas d'installation directe depuis
Chrome Web Store ou addons.mozilla.org. Tests réels du moteur encore nécessaires.
Références : https://doc.qt.io/qt-6/qwebengineextensionmanager.html et
https://developer.mozilla.org/en-US/docs/Mozilla/Add-ons/WebExtensions/Chrome_incompatibilities

## Extensions au format MiniBrowser

Ouvrez le menu Extensions, cliquez sur « Installer depuis un dossier… » et choisissez
un dossier contenant manifest.json. Sélectionnez l'extension installée, cliquez sur
Activer, puis fermez la fenêtre : les onglets ouverts se rechargent.
L'exemple `examples/reading_mode` augmente l'espacement des paragraphes dans les articles.

Format MiniBrowser uniquement : les fichiers CRX/XPI et les manifestes Chrome/Firefox
ne sont pas pris en charge. Aucun téléchargement de catalogue n'est effectué.
Les extensions installées sont copiées dans `data/installed-extensions` (dans le dossier
utilisateur pour l'application compilée), séparément des extensions livrées avec le logiciel.
Ne publiez pas ce dossier personnel.

## Manifeste

`name` et `version` : textes obligatoires. `description` : texte optionnel.
`script` : nom d'un fichier JavaScript situé directement dans le dossier (maximum 1 Mo).
`blocked_hosts` : liste optionnelle de domaines en minuscules, sans schéma ni joker.
Au moins un script ou un domaine bloqué est nécessaire. Les autres champs sont refusés.
Le script s'exécute une fois le document prêt, dans le monde isolé ApplicationWorld,
sur la page principale de chaque onglet ordinaire. Aucune API chrome.* ou browser.*.
Il peut lire/modifier le DOM et provoquer des connexions réseau : ce format ne fournit
pas un système de permissions par site. Les fenêtres privées n'utilisent pas ces extensions.

L'installation n'exécute pas le script, ignore les fichiers non déclarés et n'active
jamais automatiquement l'extension. Le même paquet ne peut pas être installé deux fois.
Une nouvelle version est installée séparément : désactivez l'ancienne avant de l'activer.
