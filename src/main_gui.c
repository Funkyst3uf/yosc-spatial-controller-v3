/*
 * Fichier : main_gui.c
 * Point d'entrée du programme de contrôle du processeur immersif Yamaha DME7
 * Auteur : Jonathan Ntoula - Juin 2026
 * Pilote le cycle de vie du programme avec interface graphique épurée.
 * Projet réalisé pour le cours Interprétation & Compilation / Logiciels Libres
 * dirigé par M. Kislin - Licence Informatique, Université Paris 8
 */

#include "sys.h"
#include "dial.h"
#include "spatial.h"
#include "audio.h"
#include "y.tab.h" /* Généré par Bison, contient les définitions de tokens */

// Inclusion de la bibliothèque de base (fenêtre, entrées, graphismes bas niveau)
#include "raylib.h"
// Cette macro active le code source des boutons et widgets de Raygui avant inclusion
#define RAYGUI_IMPLEMENTATION
#include "raygui.h"

#include <string.h>
#include <stdio.h>
#include <stdbool.h>
#include <stdlib.h>

#define MAX_HISTORY 13  // Nombre maximum de lignes visibles simultanément à l'écran dans l'historique de l'UI
#define BUFFER_SIZE 256 // Fixe la limite (en caractères) de la zone de saisie de l'UI

extern Point3D objets[];

// Descripteur fichier (socket UDP) utilisé pour la communication avec le processeur Yamaha DME7
int fd; 

int yyparse(void); // Fonction d'analyse syntaxique générée par Bison.

typedef struct yy_buffer_state *YY_BUFFER_STATE;
YY_BUFFER_STATE yy_scan_string(const char *str);
void yy_delete_buffer(YY_BUFFER_STATE buffer);

int main(int ac, char *av[]) {
    // Valeurs de connexion par défaut (vers le simulateur local)
    char target_ip[64] = "127.0.0.1";
    char target_port[16] = "50528";

    // Masque les messages de démarrage internes de Raylib
    SetTraceLogLevel(LOG_ERROR);
    printf("Demarrage du pilote DME7...\n");

    // L'utilisateur renseigne des arguments dans le terminal : ils prennent le pas
    if (ac >= 3) {
        strncpy(target_ip, av[1], sizeof(target_ip) - 1);
        strncpy(target_port, av[2], sizeof(target_port) - 1);
        printf("Arguments detectes. Connexion manuelle vers %s:%s\n", target_ip, target_port);
    } 
    // Aucun argument (double-clic) : on utilise les valeurs par défaut
    else {
        printf("Aucun argument fourni. Utilisation des valeurs par defaut (%s:%s)\n", target_ip, target_port);
    }

    // Appel de la fonction dial() avec les informations réseau définies
    fd = dial(target_ip, target_port);
    if (fd < 0) { // En cas d'échec de connexion
        perror("Erreur de connexion (dial)");
        return 1;
    }

    /* Délégation de l'initialisation des tables (objets et labels) au module spatial.c */
    init_tables(); 

    // Initialise et ouvre la fenêtre graphique avec sa taille et son titre
    InitWindow(800, 600, "Terminal de controle DME7");
    SetTargetFPS(60);

    // Règle la taille du texte par défaut pour l'interface graphique
    GuiSetStyle(DEFAULT, TEXT_SIZE, 20);

    /* Configuration esthétique monochrome */
    GuiSetStyle(TEXTBOX, BASE_COLOR_NORMAL, ColorToInt(BLACK));
    GuiSetStyle(TEXTBOX, BASE_COLOR_FOCUSED, ColorToInt(BLACK));
    GuiSetStyle(TEXTBOX, BASE_COLOR_PRESSED, ColorToInt(BLACK));
    GuiSetStyle(TEXTBOX, BASE_COLOR_DISABLED, ColorToInt(BLACK));
    GuiSetStyle(TEXTBOX, BORDER_COLOR_FOCUSED, ColorToInt(WHITE));
    GuiSetStyle(TEXTBOX, TEXT_COLOR_FOCUSED, ColorToInt(WHITE));
    GuiSetStyle(TEXTBOX, BORDER_COLOR_PRESSED, ColorToInt(LIGHTGRAY));
    GuiSetStyle(TEXTBOX, TEXT_COLOR_PRESSED, ColorToInt(LIGHTGRAY));

    char textBuffer[BUFFER_SIZE] = "\0"; // Zone mémoire stockant la frappe de l'utilisateur
    char history[MAX_HISTORY][BUFFER_SIZE + 8]; // Tableau stockant l'historique de l'écran
    
    for (int i = 0; i < MAX_HISTORY; i++) history[i][0] = '\0'; // Purge de l'historique au lancement
    
    // Message d'accueil indiquant les paramètres réseau utilisés
    snprintf(history[MAX_HISTORY - 1], BUFFER_SIZE + 8, "Connecte au DME7 [%s:%s]. Pret.", target_ip, target_port);

    // Début de la boucle de rendu asynchrone
    while (!WindowShouldClose()) {
        bool executerCommande = false; // Drapeau déterminant si une analyse doit être lancée

        if (IsKeyPressed(KEY_ENTER)) executerCommande = true; // Validation clavier (touche Entrée)

        BeginDrawing(); // Prépare le moteur graphique à dessiner la frame courante
        ClearBackground(BLACK); // Nettoie l'écran précédent avec un fond noir

        BeginScissorMode(0, 0, 750, 495); // Restreint l'affichage pour éviter que le texte ne déborde sur l'interface
        for (int i = 0; i < MAX_HISTORY; i++) {
            if (history[i][0] != '\0') {
                Color textColor = (history[i][0] == '>') ? LIGHTGRAY : WHITE; // Grise la saisie utilisateur, met en valeur les réponses
                DrawText(history[i], 25, 25 + (i * 32), 20, textColor); // Affiche la ligne d'historique à l'écran
            }
        }
        EndScissorMode(); // Fin de la zone d'affichage restreinte

        DrawLine(0, 510, 800, 510, DARKGRAY); // Ligne de séparation esthétique au-dessus de la zone de texte
        
        GuiTextBox((Rectangle){ 25, 530, 605, 40 }, textBuffer, BUFFER_SIZE, true); // Affiche et gère le champ de saisie
        
        if (GuiButton((Rectangle){ 645, 530, 130, 40 }, "Envoyer")) executerCommande = true; // Validation souris (Bouton Envoyer)

        EndDrawing(); // Finalise le rendu et affiche la frame à l'écran

        if (executerCommande && textBuffer[0] != '\0') {
            for (int i = 0; i < (MAX_HISTORY - 1); i++) {
                memmove(history[i], history[i + 1], BUFFER_SIZE + 8); // Décale l'historique vers le haut d'une ligne
            }
            
            snprintf(history[MAX_HISTORY - 1], BUFFER_SIZE + 8, "> %s", textBuffer); // Injecte la commande tapée en bas de l'écran

            bool doitQuitter = (strcmp(textBuffer, "QUIT") == 0); // Surveille la demande d'arrêt du programme
            
            char tempBuffer[BUFFER_SIZE + 2];
            snprintf(tempBuffer, sizeof(tempBuffer), "%s\n", textBuffer); // Ajoute le saut de ligne requis par le parseur Bison

            char *ptrBuffer = NULL;
            size_t sizeBuffer = 0;
                
            FILE *memStream = open_memstream(&ptrBuffer, &sizeBuffer); // Ouvre un flux dirigé vers un buffer en RAM

            FILE *old_stdout = stdout; // Sauvegarde la sortie standard d'origine
            FILE *old_stderr = stderr; // Sauvegarde la sortie d'erreur d'origine

            stdout = memStream; // Détourne les messages normaux vers la RAM
            stderr = memStream; // Détourne les messages d'erreur vers la RAM

            YY_BUFFER_STATE bp = yy_scan_string(tempBuffer); // Fournit le texte à analyser au lexer (Flex)
            yyparse(); // Lance l'analyse de la grammaire et l'exécution (Bison)
            yy_delete_buffer(bp); // Nettoie la mémoire du lexer

            fflush(memStream); // Force l'écriture des dernières données dans la RAM
            fclose(memStream); // Ferme le flux de redirection
            
            stdout = old_stdout; // Restaure la sortie standard
            stderr = old_stderr; // Restaure la sortie d'erreur

            if (ptrBuffer != NULL && sizeBuffer > 0) {
                char *line = strtok(ptrBuffer, "\n\r"); // Découpe le texte récupéré ligne par ligne
                while (line != NULL) {
                    if (strlen(line) > 0) {
                        for (int i = 0; i < (MAX_HISTORY - 1); i++) {
                            memmove(history[i], history[i + 1], BUFFER_SIZE + 8); // Décale l'historique vers le haut
                        }
                        snprintf(history[MAX_HISTORY - 1], BUFFER_SIZE + 8, "%s", line); // Injecte la réponse du parseur
                    }
                    line = strtok(NULL, "\n\r"); // Passe à la ligne suivante
                }
                free(ptrBuffer); // Libère la mémoire allouée par open_memstream
            }

            textBuffer[0] = '\0'; // Vide la zone de texte pour la commande suivante
            
            if (doitQuitter) break; // Quitte la boucle de rendu si nécessaire
        }
    }

    CloseWindow(); // Détruit le contexte graphique et ferme la fenêtre
    close(fd); // Ferme proprement la socket réseau
    
    return 0;
}