# Extensions intégrées 2.0 et personnalisation

Les trois extensions passent à 2.0.0. L’état d’activation existant est conservé ;
activer ou désactiver depuis Menu → Extensions. Les nouveaux réglages ne les
activent pas automatiquement. Aucune liste externe n’est téléchargée.

## Ad Blocker

Conserve le filtrage réseau des domaines connus et ajoute un masquage CSS ciblé
sur certains emplacements publicitaires explicites. Le CSS s’applique aussi aux
éléments ajoutés dynamiquement, sans parcours répétitif de la page.
Le masquage peut être désactivé indépendamment du filtrage réseau.
Ce n’est pas un moteur EasyList complet ni une garantie de bloquer toutes les pubs.

## Privacy Guard

Conserve le filtrage réseau et ajoute une protection des liens lors de leur
activation : retrait de l’attribut ping, noreferrer sur les liens web externes,
noopener lorsqu’ils ouvrent un nouvel onglet. Pas de modification des URL,
jetons, formulaires ou paramètres de paiement. Cette protection des liens est
optionnelle : masquer le référent peut gêner certains sites.
Elle ne bloque pas tout suivi JavaScript et ne rend pas anonyme.

## Dark Mode

Remplace l’inversion globale par une feuille de style : les images, vidéos,
canvas et SVG ne sont plus inversés. Palettes neutre/chaude et détection des
pages déjà sombres (réglable). Certains sites complexes ou contenus en iframe
peuvent rester imparfaits ; ajouter alors une exception par site.

## Exceptions

Dans Paramètres → Extensions 2.0, saisir les domaines séparés par des virgules,
sans https://, chemin ni joker. Une exception inclut les sous-domaines, pas les
domaines dont le nom ressemble simplement. Chaque extension a sa propre liste.
Le filtrage réseau s’appuie sur le site à l’origine de la requête, selon
[QWebEngineUrlRequestInfo](https://doc.qt.io/qt-6/qwebengineurlrequestinfo.html).
Recharger les pages déjà ouvertes pour actualiser les scripts et styles.
Les extensions intégrées restent appliquées au profil normal, pas aux fenêtres privées.

## Personnalisation

Paramètres → Apparence : quatre accents, taille de texte 11–18 px, visibilité
du bouton Accueil et de la barre d’état, logo sur l’accueil, 0–16 raccourcis,
suggestions issues de l’historique activables séparément. Les pages d’accueil
ouvertes se régénèrent à l’enregistrement. Les catégories peuvent défiler.

Validation : 52 tests Python et 6 tests JavaScript avec DOM simulé. Vérifier
visuellement dans Qt les réglages et quelques sites avant diffusion ; ces tests
ne garantissent pas la compatibilité de tous les sites ni du moteur natif.
