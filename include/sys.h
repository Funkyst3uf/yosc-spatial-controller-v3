/*
 * sys.h
 * En-tête principal regroupant les dépendances système et les constantes OSC
 * Auteur : Jonathan Ntoula
 * Date   : Mai 2026
 * Ce fichier centralise toutes les inclusions de bibliothèques standards 
 * ainsi que les définitions des chemins (paths) 
 * conformément aux spécifications OSC du processeur immersif Yamaha DME7.
 */

#ifndef SYS_H
#define SYS_H

#include <sys/types.h>
#include <sys/socket.h>
#include <netdb.h>
#include <netinet/in.h>
#include <stdio.h>
#include <unistd.h>
#include <ctype.h>
#include <stdlib.h>
#include <signal.h>
#include <string.h>
#include <assert.h>
#include <math.h>

#include <unistd.h> // Pour usleep 
#include <lo/lo.h>   // gestion du protocole OSC

/* * Chemins OSC DME7
 */

/* Chemin pour la position physique de l'objet (X, Y, Z) */
#define OSC_PHYSICAL_POSITION "/yosc:req/set/PROC:Component/40000/OBA/Object/PhysicalPosition/"

/* Chemin pour le contrôle du Mute (Fader On/Off) */
#define OSC_OBJECT_MUTE "/yosc:req/set/PROC:Component/40000/OBA/Object/Fader/On/"

#endif