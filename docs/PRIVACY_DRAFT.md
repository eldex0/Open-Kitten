# Confidentialité — brouillon technique avant publication

MiniBrowser conserve localement les réglages, favoris, historique et sessions.
Les profils ordinaires V9 disposent de cookies et de données web séparés ; ils ne
constituent pas une protection contre une autre personne utilisant le même compte Windows.
Dans la version installée, les données résident dans `%LOCALAPPDATA%/MiniBrowser/data`.
La désinstallation conserve ces données pour éviter une perte involontaire.

Les pages visitées contactent leurs propres serveurs. Les recherches saisies sont
envoyées au moteur choisi. La prélecture DNS est activée dans la navigation ordinaire.
La fenêtre privée ouvre actuellement Google au démarrage et utilise un profil en mémoire.
Elle ne masque pas l'adresse IP ; les fichiers téléchargés restent sur disque.

Le code applicatif inspecté ne contient pas de service de télémétrie MiniBrowser.
Les échanges du moteur Chromium et des sites n'ont pas fait l'objet d'une capture réseau
exhaustive. Ne pas annoncer « aucun suivi » ou « aucune connexion externe ».

Avant publication : compléter l'identité de l'éditeur, le contact et la politique
de l'hébergement du site ; vérifier le comportement réel de la version compilée.
Corriger le stockage en clair du mot de passe proxy avant de proposer l'authentification proxy.
