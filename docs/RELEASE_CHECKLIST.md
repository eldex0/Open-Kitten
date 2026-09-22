# Validation avant publication

Statut : NON PUBLIABLE. Scripts préparatoires uniquement, aucun installateur testé.

## Correctifs de préparation
- Ressources empaquetées séparées des données personnelles.
- Liste explicite des ressources distribuées : aucune session utilisateur.
- Validation des chemins de profils, y compris liens pointant hors du dossier.
- Profils QtWebEngine distincts, pages rattachées explicitement à leur profil.
- Blocage de domaines exacts ou sous-domaines, sans faux positifs par simple sous-chaîne.
- Scripts d'extension limités à leur dossier ; retrait ciblé des scripts MiniBrowser.

## Bloquants restants
- Installer et tester PyQt6/QtWebEngine sur l'environnement de validation.
- Figer les versions validées et vérifier les correctifs de sécurité du moteur Chromium embarqué.
- Exécuter un vrai test graphique : démarrage/fermeture, onglets, profils, mode privé,
  cookies isolés, paramètres, fenêtres secondaires et téléchargements actifs à la fermeture.
- Valider le cycle de destruction pages/profils avec Qt réel.
- Vérifier les erreurs de téléchargement, nom de fichier, progression et ouverture.
- Examiner le stockage des identifiants proxy : mot de passe actuellement dans settings.json,
  authentification proxy non implémentée. Ne pas présenter cette option comme opérationnelle.
- Valider les manifestes d'extensions de manière exhaustive avant toute installation externe.
- Vérifier plein écran vidéo, codecs et DRM avec pages publiques reproductibles.
- Compiler l'application et l'installateur ; tester installation, mise à niveau et désinstallation.
- Ajouter texte GPL, licences tierces, avis de copyright et archive source correspondante.
- Choisir un contact de support/sécurité et un hébergement de versions.
- Préparer signature de code et publication HTTPS avec sommes SHA-256.
- Finaliser politique de confidentialité selon le comportement validé et l'hébergement choisi.
- Publier le site uniquement avec un téléchargement testé ; ne pas annoncer une version disponible avant cela.

## Régression manuelle minimale
1. Installation sans Python ; première ouverture sans données du développeur.
2. Ouvrir A/B/C ; fermer B ; déplacer C ; vérifier adresse et navigation.
3. Créer deux profils : aucune connexion partagée. Fermer puis rouvrir pour persistance.
4. Mode privé : connexion absente dans une nouvelle fenêtre après fermeture complète.
5. Téléchargement interrompu, terminé, fichier absent, dossier inaccessible.
6. Thèmes clair/sombre, mise à l'échelle Windows 125/150 %, fenêtre réduite.
7. Restaurer une session, fermer avec fenêtres privées et téléchargements ouverts.
