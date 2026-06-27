/*
 * dial.c
 * Module de création et gestion de la connexion UDP.
 * Auteur : Jonathan Ntoula
 * Date   : Mai 2026
 * Ce fichier regroupe les outils nécessaires à l'établissement
 * d'une liaison IPv4/UDP (getaddrinfo(), socket(), connect() ...).
 */

#include "sys.h"
#include "dial.h"

/*
 * Initialise une socket UDP connectée à une machine et un port donnés.
 * * Cette fonction utilise getaddrinfo() pour résoudre l'adresse, puis tente
 * de créer une socket IPv4/UDP. Elle renvoie un descripteur de fichier
 * permettant l'usage de write() pour communiquer avec le serveur.
 */
int dial(char * machine, char * port) { 
    struct addrinfo * info, * p; // Liste chaînée de la configuration réseau (adresse, famille, protocole...)
    struct addrinfo indices;
    int toserver, t;

    memset(&indices, 0, sizeof indices) ;

    indices.ai_family = AF_INET; // force l'utilisation d'IPv4
    indices.ai_socktype = SOCK_DGRAM; // spécifie le mode datagramme (UDP)

    // Résolution du nom d'hôte et récupération des structures d'adresse
    t = getaddrinfo(machine, port, &indices, &info);

    if ( t != 0) // échec de résolution
        return -1;

    // trouver une adresse qui fonctionne dans les reponses ...
    for (p = info; p != 0; p = p->ai_next){
        toserver = socket(p->ai_family, p->ai_socktype, 0);
        if (toserver >= 0)
        break; // socket créée avec succès, on sort
    }

    if (p == 0) // aucune adresse n'a pu aboutir à la création d'une socket
        return -1;

    //  utilisation de connect() sur une socket UDP
    t = connect(toserver, info->ai_addr, info->ai_addrlen);

    if (t < 0) {
        freeaddrinfo(info); // on libère la mémoire

        close(toserver) ;
        return -1;
    }

    freeaddrinfo(info);

    return toserver; // on renvoie le descripteur
    
}