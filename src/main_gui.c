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

    char textBuffer[BUFFER_SIZE] = "\0";
    char history[MAX_HISTORY][BUFFER_SIZE + 8];
    
    for (int i = 0; i < MAX_HISTORY; i++) history[i][0] = '\0';
    
    // Message d'accueil indiquant les paramètres réseau utilisés
    snprintf(history[MAX_HISTORY - 1], BUFFER_SIZE + 8, "Connecte au DME7 [%s:%s]. Pret.", target_ip, target_port);

    // Début de la boucle de rendu asynchrone
    while (!WindowShouldClose()) {
        bool executerCommande = false;

        if (IsKeyPressed(KEY_ENTER)) executerCommande = true; 

        BeginDrawing();
        ClearBackground(BLACK); 

        BeginScissorMode(0, 0, 750, 495);
        for (int i = 0; i < MAX_HISTORY; i++) {
            if (history[i][0] != '\0') {
                Color textColor = (history[i][0] == '>') ? LIGHTGRAY : WHITE;
                DrawText(history[i], 25, 25 + (i * 32), 20, textColor);
            }
        }
        EndScissorMode();

        DrawLine(0, 510, 800, 510, DARKGRAY);
        
        GuiTextBox((Rectangle){ 25, 530, 605, 40 }, textBuffer, BUFFER_SIZE, true);
        
        if (GuiButton((Rectangle){ 645, 530, 130, 40 }, "Envoyer")) executerCommande = true;

        EndDrawing();

        if (executerCommande && textBuffer[0] != '\0') {
            for (int i = 0; i < (MAX_HISTORY - 1); i++) {
                memmove(history[i], history[i + 1], BUFFER_SIZE + 8);
            }
            
            snprintf(history[MAX_HISTORY - 1], BUFFER_SIZE + 8, "> %s", textBuffer);

            bool doitQuitter = (strcmp(textBuffer, "QUIT") == 0);
            
            char tempBuffer[BUFFER_SIZE + 2];
            snprintf(tempBuffer, sizeof(tempBuffer), "%s\n", textBuffer);

            char *ptrBuffer = NULL;
            size_t sizeBuffer = 0;
                
            FILE *memStream = open_memstream(&ptrBuffer, &sizeBuffer);

            FILE *old_stdout = stdout;
            FILE *old_stderr = stderr;

            stdout = memStream;
            stderr = memStream;

            YY_BUFFER_STATE bp = yy_scan_string(tempBuffer);
            yyparse();
            yy_delete_buffer(bp);

            fflush(memStream);
            fclose(memStream);
            
            stdout = old_stdout;
            stderr = old_stderr;

            if (ptrBuffer != NULL && sizeBuffer > 0) {
                char *line = strtok(ptrBuffer, "\n\r");
                while (line != NULL) {
                    if (strlen(line) > 0) {
                        for (int i = 0; i < (MAX_HISTORY - 1); i++) {
                            memmove(history[i], history[i + 1], BUFFER_SIZE + 8);
                        }
                        snprintf(history[MAX_HISTORY - 1], BUFFER_SIZE + 8, "%s", line);
                    }
                    line = strtok(NULL, "\n\r");
                }
                free(ptrBuffer); 
            }

            textBuffer[0] = '\0';
            
            if (doitQuitter) break;
        }
    }

    CloseWindow();
    close(fd);
    
    return 0;
}