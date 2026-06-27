/*
 * spatial.c
 * Module de spatialisation des objets et envoi des messages OSC
 * Auteur : Jonathan Ntoula
 * Date   : Mai 2026
 *
 * Ce module gère les algorithmes de mouvement des sources sonores dans l'espace 3D.
 * Il assure la conversion des commandes en paquets binaires OSC pour envoi au
 * processeur Yamaha DME7 via la bibliothèque liblo.
 */

#include "sys.h"
#include "spatial.h"

/* * Table de spatialisation des 64 objets (index 1 à 64). 
 * Stocke la dernière position connue de chaque objet.
 */
Point3D objets[65];         

/* * Bibliothèque de positions.
 * Permet de stocker jusqu'à 50 coordonnées cartésiennes indexées par une étiquette textuelle (label).
 */
LabelPosition table_symboles[50]; 

/* Compteur du nombre de labels actuellement enregistrés dans la bibliothèque. */
int compteur = 0;

/*
 * Initialise les données de spatialisation.
 * Réinitialise les coordonnées de tous les objets au centre de l'espace (0,0,0)
 * et reset la table des étiquettes en marquant le premier caractère des noms comme nul.
 */
void init_tables() {  // fonction ajoutée au programme précédent (UE Réseaux)
    // Initialisation des 64 objets
    for(int n = 0; n <= 64; n++) {
        objets[n].x = 0.0f;
        objets[n].y = 0.0f;
        objets[n].z = 0.0f;
    }

    // Initialisation des 50 labels
    for(int i = 0; i < 50; i++) {
        table_symboles[i].nom[0] = '\0';
        table_symboles[i].x = 0.0f;
        table_symboles[i].y = 0.0f;
        table_symboles[i].z = 0.0f;
    }
    
    compteur = 0; // nombre de label enregistré (mis à jour au fur et à mesure)
}

/*
 * Enregistre ou met à jour une position nommée dans la bibliothèque.
 * Si le nom existe déjà, les coordonnées sont mises à jour. Sinon, une nouvelle entrée est créée.
 * * Paramètres :
 * - nom : Chaîne de caractères identifiant la position.
 * - p   : Structure Point3D contenant les coordonnées cibles.
 */
void set_position_label(char* nom, Point3D p) { 
    for(int i=0; i < compteur; i++) { // met à jour un label existant ...
        if(strcmp(table_symboles[i].nom, nom) == 0) {
            table_symboles[i].x = p.x; 
            table_symboles[i].y = p.y; 
            table_symboles[i].z = p.z;
            return; // si le label existe, la fonction s'arrête ici
        }
    }

    if (compteur >= 50) {  // Jusqu'à 50 labels mémorisés (au-delà : déclenche une erreur)
        fprintf(stderr, "Erreur : Bibliothèque de labels pleine (max 50).\n");
        return; // si la table est pleine, la fonction s'arrête ici
    }

    strncpy(table_symboles[compteur].nom, nom, 31); // crée un label s'il n'existe pas déjà
    table_symboles[compteur].nom[31] = '\0'; 
    table_symboles[compteur].x = p.x;
    table_symboles[compteur].y = p.y;
    table_symboles[compteur].z = p.z;
    compteur++;
}

/*
 * Recherche les coordonnées d'une position à partir de son étiquette.
 * * Paramètres :
 * - nom : Nom de la position recherchée.
 * - x   : Pointeur vers la variable recevant la coordonnée X.
 * - y   : Pointeur vers la variable recevant la coordonnée Y.
 * - z   : Pointeur vers la variable recevant la coordonnée Z.
 *
 * Retour :
 * - 1 si le label est trouvé, 0 sinon.
 */
int get_position_by_label(char* nom, float* x, float* y, float* z) {
    for(int i=0; i < compteur; i++) { // boucle sur le nombre de labels déjà enregistrés
        if(strcmp(table_symboles[i].nom, nom) == 0) { // label trouvé
            *x = table_symboles[i].x; *y = table_symboles[i].y; *z = table_symboles[i].z; // écrit les positions aux adresses fournies
            return 1; // succès
        }
    }
    return 0; // échec
}

/*
 * Envoie une commande de positionnement absolu via OSC (téléportation).
 * Encapsule les coordonnées 3D dans un message liblo, le sérialise 
 * et l'envoie sur la socket UDP.
 * * Paramètres :
 * - fd : Descripteur de la socket UDP ouverte.
 * - id : Identifiant de l'objet audio (1-64).
 * - B  : Structure Point3D contenant les coordonnées cibles.
 */
void jump_to_position(int fd, int id, Point3D B) { 

    lo_message msg = lo_message_new();    // Création du message liblo
    lo_message_add_float(msg, B.x); // ajout d'un float au message ...
    lo_message_add_float(msg, B.y);  // ... d'un deuxième
    lo_message_add_float(msg, B.z);  // ... et d'un troisième comme attendu

    char path[128]; // création du chemin OSC initialement gérée par le main()
    snprintf(path, 128, OSC_PHYSICAL_POSITION "%d", id);

    size_t taille;  // taille du buffer
    
    // Sérialisation (fabrication du buffer binaire)
    // alloue et remplit un buffer binaire conforme au protocole OSC
    void *buffer = lo_message_serialise(msg, path, NULL, &taille);

    // Envoi du message via la socket (fd) créée par la fonction dial()
    if (buffer != NULL) {
        write(fd, buffer, taille);
        free(buffer); 
    }

    objets[id] = B; // mise à jour de la position de l'objet en mémoire

    lo_message_free(msg); // libération de la mémoire allouée par liblo
}

/*
 * Déplace un objet de manière continue par interpolation linéaire (LERP).
 * Découpe la trajectoire entre A et B en une série de pas. 
 * La fluidité est assurée par un pas fixe de 4 cm.
 * * Paramètres :
 * - fd   : Descripteur de la socket UDP.
 * - id   : Identifiant de l'objet (1-64).
 * - A    : Point de départ (position actuelle).
 * - B    : Point d'arrivée.
 * - time : Durée totale du mouvement en secondes.
 */
void move_to_position(int fd, int id, Point3D A, Point3D B, float time) { 

    float step_size = 0.04;   // fixe la largeur du pas (ici, 4 cm pour une grande fluidité)

    // calcul des deltas et de la distance totale
    float dx = B.x - A.x;
    float dy = B.y - A.y;
    float dz = B.z - A.z;

    // calcul de la distance globale totale 
    float total_dist = sqrt(dx*dx + dy*dy + dz*dz);

    if (total_dist < 0.001) return; 

    // calcul du nombre de pas nécessaire en fonction de la distance
    int total_steps = (int)(total_dist / step_size);
    if (total_steps < 1) total_steps = 1; 

    // Calcul du délai entre chaque itération
    float delay_sec = time / total_steps; // x seconde pour faire Y pas
    long delay_usec = (long)(delay_sec * 1000000); // conversion en microseconde pour usleep

    Point3D gps;   // pour les coordonnées de transition entre A et B

    // envoi des coordonnées intermédiaires en boucle à jump_to_position()
    // sur n nombre de pas (step)
    for (int i = 0; i <= total_steps; i++) {
        // t = facteur commun pour chaque composante des coordonnées 
        float t = (float)i / (float)total_steps; // avec 0 <= t <= 1
        
        gps.x = A.x + t * dx; // chaque composante "évolue" au même rythme
        gps.y = A.y + t * dy; 
        gps.z = A.z + t * dz;

        jump_to_position(fd, id, gps); // envoi des coordonnées intermédiaires à jump()

        if (i < total_steps) usleep(delay_usec); // temps d'attente entre chaque envoi
    }
}