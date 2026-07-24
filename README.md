# YOSC Spatial Controller (v3) - Contrôleur Immersif Yamaha DME7

Ce projet est un contrôleur spatial doté d'une interface graphique légère (GUI) écrite en C, conçu pour piloter le moteur de rendu audio spatial du processeur immersif **Yamaha DME7**.

À l'aide d'un analyseur lexical et syntaxique robuste généré par **Flex et Bison**, ce contrôleur traduit un langage de script dédié en paquets réseau binaires. Ces paquets sont transmis en temps réel au DME7 via le protocole **OSC (Open Sound Control) sur UDP**. L'interface graphique offre un environnement de travail dédié.

> [!NOTE]
> ** Accès aux paquets précompilés (.deb)**
> Suite à l'automatisation de la compilation et du packaging sous **GitHub Actions**, les paquets Debian générés se trouvent dans l'onglet **Actions** de ce dépôt. 
> Pour les télécharger : cliquez sur le dernier *workflow* exécuté avec succès, puis descendez tout en bas de la page dans la section **Artifacts**.

---

## 1. Stack Technique

* **Langage :** C (Norme C99 et extensions système POSIX)
* **Interface Graphique :** Raylib & Raygui (Mode Immédiat, rendu 60 Hz)
* **Analyse syntaxique/lexicale :** GNU Bison & Flex
* **Communication réseau :** Sockets UDP (POSIX) via la bibliothèque `liblo` (encapsulation OSC)
* **Build System :** Makefile unifié (gestion des cibles locales et du packaging d'industrialisation)

---

## 2. Architecture et Topologie du Dépôt

L'organisation des fichiers respecte une structure standardisée pour isoler proprement le code écrit, les outils de configuration et les scripts de déploiement :

```text
yosc-spatial-controller-v3/
├── include/              # Fichiers d'en-tête fixes (.h) et raygui.h
├── src/                  # Codes sources (.c), Flex (.l) et Bison (.y)
│   └── dme7_simulator.c  # Simulateur réseau pour les tests en autonomie
├── docs/                 # Spécifications détaillées (Grammaire, protocole OSC)
├── spatial-controller_1.0.0_amd64/
│   └── DEBIAN/           # Fichier control et staging pour le paquet Debian
├── .gitignore            # Filtre d'exclusion (ignore les fichiers objets et binaires)
├── Makefile              # Automate de compilation unifiée (v1 et v2)
└── README.md             # Présente documentation (Vitrine opérationnelle du projet)

```

---

## 3. Compilation locale (Pour les développeurs)

Cette section s'adresse aux développeurs souhaitant modifier le code source ou recompiler le projet manuellement.

### Dépendances requises

Pour compiler le projet localement sur un système Linux (Ubuntu/Debian), installez les paquets de développement suivants :

```bash
sudo apt update
sudo apt install build-essential liblo-dev libraylib-dev

```

### Cibles du Makefile

* `make` ou `make all` : Compile l'ensemble du projet (l'application graphique et le simulateur réseau de test).
* `make client` : Compile uniquement le contrôleur graphique autonome (`spatial-controller`). C'est ce binaire qui est distribué dans le paquet de production.
* `make server` : Compile uniquement le simulateur réseau (`dme7-sim`). Ce binaire permet de valider le comportement de l'application sans matériel physique réel.
* `make package` : Génère localement le paquet Debian de production à partir des sources compilées.
* `make clean` : Nettoie l'espace de travail en supprimant le répertoire des objets (`obj/`) et les binaires locaux.
* `make re` : Déclenche une reconstruction complète et propre du projet.

---

## 4. Installation et Utilisation (Pour l'utilisateur final)

Pour l'ingénieur système sur site, le déploiement s'effectue proprement via le gestionnaire natif, sans avoir besoin du code source ni de compiler quoi que ce soit.

### Installation via le paquet Debian

1. Récupérez le paquet précompilé `spatial-controller_1.0.0_amd64.deb` (généré automatiquement et disponible dans les artefacts GitHub Actions).
2. Installez-le en utilisant `apt` depuis votre terminal (cette commande garantit le téléchargement automatique des dépendances réseau nécessaires comme `liblo7`) :

```bash
sudo apt install ./spatial-controller_1.0.0_amd64.deb

```

### Exécution

Une fois installé globalement sur le système, le pilote se lance simplement depuis n'importe quel terminal. Le contrôleur s'adapte à vos besoins via une logique d'arguments facultatifs :

```bash
# 1. Lancement local par défaut (Double-clic ou terminal)
# Pointe automatiquement vers le simulateur (127.0.0.1 sur le port 50528)
spatial-controller

# 2. Lancement ciblé (Manuel)
# Cibler un processeur physique distant en forçant l'IP et le port
spatial-controller 192.168.1.50 50528

```

### Sémantique de l'interpréteur (DSL)

Saisissez vos requêtes directement dans la zone de texte de l'interface graphique. Les commandes peuvent être chaînées en les séparant par des points-virgules (les logs de validation et erreurs du parseur sont capturés en RAM via `open_memstream()` puis affichés dans l'historique de la fenêtre) :

* `SET OBJ[1] X=0.5 Y=-0.2;` -> Mémorise les coordonnées d'un objet audio.
* `JUMP OBJ[1] X=0.0 Y=0.0;` -> Téléporte instantanément un objet.
* `MOVE OBJ[1] X=1.0 Y=1.0 T=2000;` -> Déplacement fluide et continu sur une durée spécifiée.
* `SWAP OBJ[1] OBJ[2]; PING;` -> Permute deux sources et interroge la liaison réseau.
* `MUTE OBJ[1] ON;` -> Active ou désactive la coupure audio d'un canal.

---

## 5. Documentation Complémentaire et Intégration Continue

* **Dossier `docs/` :** Vous trouverez dans ce sous-répertoire les documents d'approfondissement techniques (la sémantique détaillée de la grammaire Bison et le dictionnaire complet des messages réseau OSC du Yamaha DME7).
* **GitHub Actions :** Ce dépôt intègre un pipeline automatisé (`build.yml`) qui teste l'installation et la construction du paquet sur une matrice multi-OS (Ubuntu 22.04, 24.04 et 26.04) à chaque commit.
