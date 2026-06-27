/*
 * audio.h
 * Interface de contrôle des paramètres audio (Mute).
 * Auteur : Jonathan Ntoula
 * Date   : Mai 2026
 * Ce fichier contient les prototypes des fonctions permettant d'agir sur les 
 * paramètres audio (notamment le Mute) des objets du moteur immersif DME7.
 * Il assure le lien entre le parser et le module d'implémentation audio.c.
 */

#ifndef AUDIO_H
#define AUDIO_H

/*
 * Modifie l'état de mute (ON/OFF) d'un objet audio.
 *
 * Cette fonction construit et envoie le message OSC correspondant
 * au paramètre de "Mute" pour un objet spécifique identifié par son ID.
 *
 * Paramètres :
 * - fd   : Descripteur de la socket UDP ouverte.
 * - id   : Identifiant numérique de l'objet (index 1 à 64).
 * - mute : Entier représentant l'état (1 pour actif/silence, 0 pour inactif/diffusion).
 */
void set_mute(int fd, int id, int mute);

#endif