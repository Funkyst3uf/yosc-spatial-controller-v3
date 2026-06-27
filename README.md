# DME7 Spatial Controller & Interface Graphique
Ce projet est un contrôleur spatial doté d'une interface graphique légère (GUI) écrite en C, conçu pour piloter le moteur de rendu audio spatial du processeur immersif **Yamaha DME7**. 

## Le Projet
À l'aide d'un analyseur lexical et syntaxique robuste généré par **Flex et Bison**, ce contrôleur traduit un langage de script dédié (DSL) en paquets réseau binaires. Ces paquets sont ensuite sérialisés et transmis en temps réel au DME7 via le protocole **OSC (Open Sound Control) sur UDP**.

Le projet intègre désormais un monitoring visuel en temps réel grâce à une interface graphique légère et performante.

## Stack Technique
* **Langage :** C (Norme C99)
* **Interface Graphique :** Raylib & Raygui
* **Analyse syntaxique/lexicale :** GNU Bison & Flex
* **Communication réseau :** Sockets UDP (POSIX)
* **Protocole :** OSC (via la bibliothèque `liblo`)
* **Build System :** Makefile (Architecture sectorisée `src/`, `include/`, `obj/`)

## Fonctionnalités du Langage & UI
Le contrôleur permet d'orchestrer et de visualiser 64 objets audio dans un espace 3D :
* `SET` : Mémorisation de coordonnées dans l'espace (x, y, z).
* `JUMP` : Téléportation instantanée d'un objet.
* `MOVE` : Déplacement continu et fluide avec gestion du temps de trajet.
* `SWAP` : Permutation croisée des positions entre deux sources.
* `PONG` : Séquences d'allers-retours automatisées.
* `MUTE` : (Dés)activation audio binaire (Mute ON/OFF).

## Architecture du Projet
```text
.
├── Makefile        # Système de build automatisé
├── assets/         # Polices et ressources graphiques de l'UI
├── include/        # Headers fixes (.h)
└── src/            # Sources d'origine (.c, .l, .y) et générés
