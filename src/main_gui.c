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

#define MAX_HISTORY 13  
#define BUFFER_SIZE 256

extern Point3D objets[];

// Descripteur fichier (socket UDP) utilisé pour la communication avec le processeur Yamaha DME7
int fd; 

int yyparse(void); // Fonction d'analyse syntaxique générée par Bison.

typedef struct yy_buffer_state *YY_BUFFER_STATE;
YY_BUFFER_STATE yy_scan_string(const char *str);
void yy_delete_buffer(YY_BUFFER_STATE buffer);

int main(int ac, char *av[]) {
    if (ac < 3) {
        fprintf(stderr, "Usage: %s <IP> <PORT>\n", av[0]);
        return 1;
    }

    // Masque les messages de démarrage internes de Raylib pour ne garder que les erreurs graves
    SetTraceLogLevel(LOG_ERROR);
    printf("Démarrage du pilote DME7 (Version avec UI autonome)...\n");

    // Appel de la fonction dial() avec les informations saisies par l'utilisateur
    fd = dial(av[1], av[2]);
    if (fd < 0) { // En cas d'échec de connexion
        perror("Erreur de connexion (dial)");
        return 1;
    }

    /* Délégation de l'initialisation des tables (objets et labels) au module spatial.c */
    init_tables(); 

    // Initialise et ouvre la fenêtre graphique avec sa taille et son titre
    InitWindow(800, 600, "Terminal de controle DME7");
    // Bloque la vitesse de rafraîchissement de la boucle à 60 images par seconde
    SetTargetFPS(60);

    // Règle la taille du texte par défaut pour l'interface graphique (Raygui utilisera sa police interne)
    GuiSetStyle(DEFAULT, TEXT_SIZE, 20);

    /* Configuration esthétique monochrome (application du style épuré noir et blanc) */
    GuiSetStyle(TEXTBOX, BASE_COLOR_NORMAL, ColorToInt(BLACK));
    GuiSetStyle(TEXTBOX, BASE_COLOR_FOCUSED, ColorToInt(BLACK));
    GuiSetStyle(TEXTBOX, BASE_COLOR_PRESSED, ColorToInt(BLACK));
    GuiSetStyle(TEXTBOX, BASE_COLOR_DISABLED, ColorToInt(BLACK));
    GuiSetStyle(TEXTBOX, BORDER_COLOR_FOCUSED, ColorToInt(WHITE));
    GuiSetStyle(TEXTBOX, TEXT_COLOR_FOCUSED, ColorToInt(WHITE));
    GuiSetStyle(TEXTBOX, BORDER_COLOR_PRESSED, ColorToInt(LIGHTGRAY));
    GuiSetStyle(TEXTBOX, TEXT_COLOR_PRESSED, ColorToInt(LIGHTGRAY));

    char textBuffer[BUFFER_SIZE] = "\0";
    char history[MAX_HISTORY][BUFFER_SIZE + 8];
    
    for (int i = 0; i < MAX_HISTORY; i++) history[i][0] = '\0';
    strcpy(history[MAX_HISTORY - 1], "Systeme connecte au DME7. Pret a envoyer des commandes.");

    // Début de la boucle de rendu asynchrone (s'exécute 60 fois par seconde)
    while (!WindowShouldClose()) {
        bool executerCommande = false;

        // Détecte si la touche Entrée a été pressée sur le clavier de la machine
        if (IsKeyPressed(KEY_ENTER)) executerCommande = true; 

        // Prépare le système à dessiner l'image actuelle (ouverture du canvas)
        BeginDrawing();

        // Efface l'écran précédent en appliquant un fond noir uni
        ClearBackground(BLACK); 

        // Délimite une zone de découpe pour empêcher le texte de déborder en bas de l'écran
        BeginScissorMode(0, 0, 750, 495);
        for (int i = 0; i < MAX_HISTORY; i++) {
            if (history[i][0] != '\0') {
                Color textColor = (history[i][0] == '>') ? LIGHTGRAY : WHITE;
                // Dessine une ligne d'historique en utilisant la police de base par défaut intégrée à Raylib
                DrawText(history[i], 25, 25 + (i * 32), 20, textColor);
            }
        }
        // Ferme la zone de découpe graphique
        EndScissorMode();

        // Dessine une ligne horizontale grise pour séparer l'historique de la zone de saisie
        DrawLine(0, 510, 800, 510, DARKGRAY);
        
        // Affiche la zone de saisie de texte en "Mode Immédiat" (met directement à jour textBuffer)
        GuiTextBox((Rectangle){ 25, 530, 605, 40 }, textBuffer, BUFFER_SIZE, true);
        
        // Affiche le bouton "Envoyer" en "Mode Immédiat" et passe la condition à vrai s'il est cliqué
        if (GuiButton((Rectangle){ 645, 530, 130, 40 }, "Envoyer")) executerCommande = true;

        // Fin des instructions de dessin : envoie le tout à la carte graphique pour affichage
        EndDrawing();

        // Condition de déclenchement : on vérifie si l'action executerCommande est vraie 
        // et que le texte saisi dans le champ (textBuffer) n'est pas vide.
        if (executerCommande && textBuffer[0] != '\0') {
        
            // Gestion de l'historique (Défilement vers le haut)
            for (int i = 0; i < (MAX_HISTORY - 1); i++) {
                // memmove déplace le contenu de la case i+1 vers la case i.
                memmove(history[i], history[i + 1], BUFFER_SIZE + 8);
            }
            
            // Ajout de la nouvelle commande à la fin de l'historique avec le préfixe prompt "> "
            snprintf(history[MAX_HISTORY - 1], BUFFER_SIZE + 8, "> %s", textBuffer);

            // Détection de la commande de sortie
            bool doitQuitter = (strcmp(textBuffer, "QUIT") == 0);
            
            // Préparation du flux pour le parseur (Ajout du retour à la ligne requis par la grammaire)
            char tempBuffer[BUFFER_SIZE + 2];
            snprintf(tempBuffer, sizeof(tempBuffer), "%s\n", textBuffer);

            // Initialisation des pointeurs système pour la redirection en RAM
            char *ptrBuffer = NULL;
            size_t sizeBuffer = 0;
                
            // Ouvre le fichier virtuel en RAM pour intercepter les flux
            FILE *memStream = open_memstream(&ptrBuffer, &sizeBuffer);

            FILE *old_stdout = stdout;
            FILE *old_stderr = stderr;

            // Redirection des sorties textuelles du système (printf, yyerror) vers la RAM
            stdout = memStream;
            stderr = memStream;

            // Branchement de la chaîne sur le Lexer (Flex)
            YY_BUFFER_STATE bp = yy_scan_string(tempBuffer);
            
            // Lancement de l'analyse syntaxique (Bison)
            yyparse();
            
            // Libération de la mémoire de Flex
            yy_delete_buffer(bp);

            // Force l'écriture des buffers et actualise ptrBuffer et sizeBuffer
            fflush(memStream);
            
            // Fermeture du flux virtuel
            fclose(memStream);
            
            // Restauration immédiate des sorties standards d'origine
            stdout = old_stdout;
            stderr = old_stderr;

            // Vérification et extraction des logs capturés en RAM
            if (ptrBuffer != NULL && sizeBuffer > 0) {
                // Découpage du texte récupéré ligne par ligne
                char *line = strtok(ptrBuffer, "\n\r");
                
                while (line != NULL) {
                    if (strlen(line) > 0) {
                        // Décalage de l'historique vers le haut
                        for (int i = 0; i < (MAX_HISTORY - 1); i++) {
                            memmove(history[i], history[i + 1], BUFFER_SIZE + 8);
                        }
                        // Copie du log dans le dernier slot de l'historique
                        snprintf(history[MAX_HISTORY - 1], BUFFER_SIZE + 8, "%s", line);
                    }
                    line = strtok(NULL, "\n\r");
                }
                // Libération de la zone mémoire allouée par open_memstream
                free(ptrBuffer); 
            }

            // Réinitialisation complète du champ de saisie de l'UI
            textBuffer[0] = '\0';
            
            // Sortie de la boucle principale si la commande était "QUIT"
            if (doitQuitter) break;
        }
    }

    // Ferme proprement la fenêtre graphique et libère le contexte Raylib
    CloseWindow();

    /* Fermeture propre de la ressource réseau */
    close(fd);
    
    return 0;
}