/*
 * dme7_simulator.c
 * Serveur OSC (substitut du processeur Yamaha DME7)
 * Auteur : Jonathan Ntoula
 * Date   : Mai 2026
 * Ce module simule la réception de messages de spatialisation et de contrôle
 * provenant d'un client OSC, en utilisant les outils de la bibliothèque liblo.
 */

#include "sys.h"
#include <time.h>

/*
 * Traitement des messages OSC entrants.
 *
 * Cette fonction est appelée à chaque réception de message OSC.
 * Elle intègre un mécanisme de contrôle temporel pour limiter 
 * l'affichage en console en cas de flood.
 *
 * Paramètres :
 * - path      : L'adresse OSC reçue (ex: /yosc:req/...).
 * - types     : La chaîne de caractères décrivant les types d'arguments (ex: "fff").
 * - argv      : Tableau de pointeurs vers les valeurs des arguments.
 * - argc      : Nombre d'arguments reçus.
 * - data      : Message OSC brut (non utilisé ici).
 * - user_data : Pointeur vers des données utilisateur (non utilisé ici).
 *
 * Retour :
 * - Retourne toujours 0 pour confirmer le traitement du message à liblo.
 */
int spatial_handler(const char *path, const char *types, lo_arg **argv,
                    int argc, lo_message data, void *user_data) {

    // Variable statique pour mémoriser "l'heure" du DERNIER AFFICHAGE
    static time_t last_time = 0;
    time_t now = time(NULL); // Heure Unix : secondes écoulées depuis le 01/01/1970

    // On calcule l'écart entre "maintenant" et le "dernier affichage"
    if (difftime(now, last_time) >= 0) { // on affiche seulement si la différence est >= 1.0 (modifiable)
    
        // Affichage des informations reçues
        if (argc >= 3 && types[0] == 'f') { // si 3 valeurs de type float = coordonnées
            printf("ADRESSE REÇUE : <%s>\n", path);
            printf("TYPES REÇUS    : [%s]\n", types);
            printf("VALEURS        : X=%.2f Y=%.2f Z=%.2f\n", argv[0]->f, argv[1]->f, argv[2]->f);
            last_time = now;
        }

        else if (argc >=1 && types[0] == 'i') { // si valeur de type int = gestion du MUTE
            printf("ADRESSE REÇUE : <%s>\n", path);
            printf("TYPES REÇUS    : [%s]\n", types);
            printf("VALEUR       : MUTE=%i\n", argv[0]->i);
        } 

        printf("----------------------------\n");

    }

    return 0;
}

/*
 * Callback en cas d'erreur liblo.
 *
 * Paramètres :
 * - num  : Code d'erreur.
 * - m    : Message d'erreur textuel.
 * - path : Chemin ou adresse ayant provoqué l'erreur.
 */
void error(int num, const char *m, const char *path) {
    printf("Erreur Liblo %d sur %s : %s\n", num, path, m);
}

/*
 * Point d'entrée principal du simulateur.
 *
 * Initialise le serveur OSC, enregistre les méthodes de routage et lance
 * le thread en arrière-plan.
 *
 * Retour :
 * - Code de sortie du programme.
 */
int main() {
    char *server_port = "50528"; // numero de port sur lequel écoute le serveur de test (même port que le DME7)

    // gérant des appels système socket() / bind() et création d'un thread 
    lo_server_thread simulator = lo_server_thread_new(server_port, error);

    lo_server_thread_add_method(simulator, NULL, NULL , spatial_handler, NULL); // accepte toutes les chemins osc et type de données
    
    lo_server_thread_start(simulator); // démarrage du serveur

    // On affiche le port utilisé
    printf("Simulateur DME7 en ligne sur le port %s...\n", server_port);
    printf("En attente de commandes OSC\n\n");

    fflush(stdout); // force l'affichage des printf() avant d'entrer dans la boucle infinie

    while (1) {   // boucle de maintien
        usleep(1000); 
    }

    lo_server_thread_free(simulator); // tout-en-un : ferme le thread, la socket et libere la mémoire
    return 0;
}