# Open Kitten 1.0.0

## Renommage compatible

Nom public, titre principal, accueil, splash, icône et sortie de compilation :
Open Kitten. Version affichée : 1.0.0. La classe Python MiniBrowser, les identifiants
de scripts, le fichier packaging/MiniBrowser.spec et le dossier de données
historique restent inchangés pour éviter de casser les intégrations et les profils.
L’AppId de l’installateur est également conservé pour identifier la même application.

Construction : `python -m PyInstaller packaging/MiniBrowser.spec` produit
`dist/OpenKitten/OpenKitten.exe`. Distribuer tout le dossier, pas seulement l’exécutable.
La compilation et la validation graphique restent à effectuer sur Windows.

## Langues : couverture réelle

Paramètres → Général sépare deux choix :

- Interface : français, anglais, allemand, espagnol, italien, portugais,
  néerlandais, arabe, chinois et japonais. Redémarrage requis.
- Langue demandée aux sites : catalogue de locales fourni par Qt, sans limiter
  ce choix aux dix langues de l’interface. Un site reste libre de ne pas le respecter.

Les 234 textes du catalogue de l’application disposent désormais d’une traduction
dans chacune des dix langues proposées : menus, paramètres, confirmations,
téléchargements, messages de validation et extensions intégrées. L’arabe utilise
une disposition droite-à-gauche. Le chinois proposé est simplifié.
Les noms de profils, fichiers, sites et extensions tierces restent ceux de leurs
auteurs ; les diagnostics bruts de Windows/Qt restent dans leur langue d’origine.
Le contenu des pages web n’est pas traduit automatiquement.

« Automatique (langue du PC) » est la valeur par défaut pour les nouvelles
installations. Les langues préférées du système sont examinées dans l’ordre,
avec reconnaissance des variantes régionales et repli sur l’anglais si aucune
langue prise en charge ne correspond. Un choix manuel enregistré reste prioritaire.
Pour une installation existante, sélectionner Automatique dans Paramètres → Général
puis redémarrer. Aucun réglage existant n’est écrasé pendant la migration.

Aucun service de traduction distant n’est utilisé. Les catalogues Python sont
inclus dans la compilation. Les contrôles Qt chargent les catalogues officiels
installés avec Qt lorsqu’ils sont disponibles ; les boutons standard sont aussi
traduits par l’application. Voir [QTranslator](https://doc.qt.io/qt-6/qtranslator.html).
La validation visuelle et linguistique par des locuteurs reste à effectuer,
notamment pour les textes longs, l’arabe et les dialogues natifs du système.

Tests : 66 tests Python et 6 tests JavaScript. Les contrôles de localisation
vérifient la couverture du catalogue, les paramètres insérés, les sauts de ligne,
la sélection automatique, les variantes régionales et la priorité du choix manuel.

## Recherche et fenêtres du menu

Sept moteurs partagés entre accueil, barre d’adresse et navigation privée :
Google, Bing, DuckDuckGo, Brave Search, Ecosia, Qwant et Startpage.
Le paramètre query de Startpage est distinct du paramètre q des autres moteurs.
Références : [Brave](https://search.brave.com/), [Ecosia](https://www.ecosia.org/),
[Qwant](https://www.qwant.com/), [Startpage](https://www.startpage.com/).
La disponibilité de chaque moteur dépend aussi de ses restrictions réseau.

Historique et favoris : panneau commun, titres et adresses sur deux lignes,
recherche temporisée, boutons regroupés, état vide et actions désactivées sans
sélection. Effacement de tout l’historique avec confirmation. Espacements communs
dans paramètres, profils et gestionnaires d’extensions.

La version 1.0.0 est un numéro demandé, pas une certification de sécurité ou une
garantie d’absence de bugs. Les prérequis de publication du README restent valables.
