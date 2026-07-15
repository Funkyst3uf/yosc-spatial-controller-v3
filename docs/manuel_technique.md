# Manuel Technique et Architecture - YOSC Spatial Controller

Ce document s'adresse aux développeurs, mainteneurs et contributeurs souhaitant comprendre l'architecture logicielle du projet, le modifier ou le compiler depuis ses sources.

## 1. Architecture Logicielle

Le projet repose sur la séparation stricte de trois couches d'ingénierie :

1. **Le Module Réseau (liblo / Sockets POSIX) :** Il constitue le véritable cœur du projet. Il se charge de traduire les structures de données C en paquets UDP/OSC formatés, en respectant scrupuleusement le dictionnaire de trames officiel du Yamaha DME7.
2. **L'Analyseur Lexico-Syntaxique (Flex / Bison) :** Il agit en amont du module réseau. Il lit la chaîne de caractères issue de l'interface graphique, valide sa conformité par rapport à notre grammaire formelle, et la convertit en structures C exploitables pour l'envoi réseau.
3. **Le Moteur Graphique (Raylib / Raygui) :** Géré en mode immédiat. Il capture les saisies utilisateur et affiche les retours via une redirection de flux mémoire (`open_memstream`) pour intercepter la sortie standard du parseur C sans bloquer l'application.

## 2. Compilation Manuelle

Si vous clonez ce dépôt pour le modifier, vous aurez besoin des en-têtes de développement.

Voici la procédre à suivre pour les installer sur votre système :

```bash
sudo apt update
sudo apt install build-essential liblo-dev libraylib-dev flex bison

```

Le projet utilise un `Makefile` :

* `make all` : Compile le client graphique (`spatial-controller`) et le serveur de test local (`dme7-sim`).
* `make client` : Compile uniquement le client graphique.
* `make server` : Compile uniquement le serveur de test local.
* `make package` : Construit l'arborescence Debian et génère le fichier `.deb` d'installation prêt pour la production.
* `make tomato` : Génère le fichier d'états de l'automate Bison (`y.output`), utile pour le débogage de la grammaire formelle.
* `make clean` : Nettoie les fichiers objets (`obj/`)
* `make re` :  Force une recompilation totale.

## 3. La Grammaire (Bison)

Le langage a été défini pour faciliter la syntaxe yOSC et éviter toute erreur de typage ou de logique au moment de l'envoi réseau. 

Le parseur agit comme un bouclier sémantique avant la génération des trames OSC.

Depuis le fichier yosc.y, la grammaire vérifie et rejette les erreurs de saisie. Par exemple, l'identifiant d'un objet audio est strictement restreint aux capacités physiques du processeur Yamaha (1 à 64) :

```yacc
object_id:
    YINT {
        if ($1 < 1 || $1 > 64) {
            fprintf(stderr, "\n[ERREUR SÉMANTIQUE] ID %d hors limites.\n", $1);             
            YYERROR; /* Interrompt et annule la commande courante */         
            }         
            $$ = $1;
    }
    ;
```
Une fois les arguments validés (comme des temps de trajet positifs, ou des états binaires 0/1 pour le MUTE), la règle de production globale peut déclencher l'action en toute sécurité. 

Exemple avec un déplacement continu (`MOVE) vers des coordonnées explicites :

```yacc
move_cmd:
    YMOVE object_id coordinates travel_time
    { 
        // Appel de la fonction réseau d'interpolation
        move_to_position(fd, $2, objets[$2], $3, $4);
    }
    ;
```


## 4. Compilation et Création Automatique des Paquets

Le dépôt intègre un processus automatisé via **GitHub Actions** (défini dans le fichier `.github/workflows/build.yml`). 

Concrètement, à chaque fois que du nouveau code est envoyé sur le dépôt, des serveurs distants s'occupent de vérifier que l'application se compile correctement et fabriquent les fichiers d'installation. Cela garantit que le code fonctionne dans un environnement strict, tout comme sur le PC d'un utilisateur final.

Pour s'assurer d'une compatibilité maximale, l'application est testée simultanément sur trois générations d'Ubuntu (les versions LTS 22.04, 24.04 et 26.04).

Voici ce qui se passe automatiquement en arrière-plan : 

1. **Préparation :** Trois machines virtuelles Ubuntu vierges démarrent.
2. **Installation :** Les outils et dépendances nécessaires sont téléchargés.
3. **Vérification :** Le code est compilé pour s'assurer qu'il n'y a aucune erreur.
4. **Distribution :** Les paquets d'installation (`.deb`) sont générés et mis à disposition dans l'onglet **Actions** de GitHub.
