/*
 * audio.c
 * Fonctions de traitement des paramètres audio via OSC.
 * Auteur : Jonathan Ntoula
 * Date   : Mai 2026
 * * Ce module contient la logique permettant de piloter l'état de diffusion
 * (Mute ON/OFF) d'un objet audio du moteur immersif DME7. 
 */

#include "sys.h"
#include "audio.h"

/*
 * Active ou désactive le son (Mute) d'un objet audio.
 *
 * Cette fonction construit un message OSC contenant l'état binaire souhaité, 
 * puis génère dynamiquement le chemin cible (le motif d'adresse OSC) en fonction 
 * de l'ID fourni. Le message est ensuite sérialisé en un bloc binaire et transmis 
 * au processeur via la socket UDP existante.
 *
 * Paramètres :
 * - fd   : Descripteur de la socket UDP ouverte (créée préalablement).
 * - id   : Identifiant unique de l'objet audio cible (1-64).
 * - mute : Etat souhaité : 1 pour activer le mute (silence), 0 pour le désactiver.
 */
void set_mute(int fd, int id, int mute) {

    // Création du message liblo
    lo_message msg = lo_message_new(); 

    lo_message_add_int32(msg, mute);

    char path[128]; // création du chemin OSC (initialement gérée par le main())
    snprintf(path, 128, OSC_OBJECT_MUTE "%d", id);

    size_t taille;

    // Sérialisation (fabrication du buffer binaire)
    // Cette fonction alloue et remplit un buffer binaire conforme au protocole OSC
    void *buffer = lo_message_serialise(msg, path, NULL, &taille);

    // Envoi du message (buffer) via la socket (fd) creee par la fonction dial()
    if (buffer != NULL) {
        write(fd, buffer, taille);
        
        // Libération de la mémoire allouée au buffer par liblo
        free(buffer);
    }

    // Nettoyage 
    lo_message_free(msg);
}