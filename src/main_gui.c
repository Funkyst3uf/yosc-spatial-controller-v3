/*
 * Fichier : main_gui.c
 * Point d'entrée du programme de contrôle du processeur immersif Yamaha DME7
 * Auteur : Jonathan Ntoula - Juin 2026
 
 * Pilote le cycle de vie du programme avec interface monochrome épurée.
 * Projet réalisé pour le cours Interprétation & Compilation / Logiciels Libres
 * dirigé par M. Kislin - Licence Informatique, Université Paris 8
 */

#include "sys.h"
#include "dial.h"
#include "spatial.h"
#include "audio.h"
#include "y.tab.h" 

#include "raylib.h"
#define RAYGUI_IMPLEMENTATION
#include "raygui.h"

#include <string.h>
#include <stdio.h>
#include <stdbool.h>
#include <stdlib.h>

#define MAX_HISTORY 13  
#define BUFFER_SIZE 256

extern Point3D objets[];
int fd; 

int yyparse(void);

typedef struct yy_buffer_state *YY_BUFFER_STATE;
YY_BUFFER_STATE yy_scan_string(const char *str);
void yy_delete_buffer(YY_BUFFER_STATE buffer);

int main(int ac, char *av[]) {
    if (ac < 3) {
        fprintf(stderr, "Usage: %s <IP> <PORT>\n", av[0]);
        return 1;
    }

    SetTraceLogLevel(LOG_ERROR);
    printf("Démarrage du pilote DME7 (Version Interface Graphique Épurée)...\n");

    fd = dial(av[1], av[2]);
    if (fd < 0) { 
        perror("Erreur de connexion (dial)");
        return 1;
    }

    init_tables(); 

    InitWindow(800, 600, "Spatial Project LL - Terminal DME7");
    SetTargetFPS(60);

    int codepoints[256];
    for (int i = 0; i < 256; i++) codepoints[i] = i;
    Font cleanMonoFont = LoadFontEx("assets/fonts/UbuntuMono-R.ttf", 24, codepoints, 256);
    
    GuiSetFont(cleanMonoFont);
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
    strcpy(history[MAX_HISTORY - 1], "Système connecté au DME7. Prêt pour les commandes.");

    while (!WindowShouldClose()) {
        bool executerCommande = false;

        if (IsKeyPressed(KEY_ENTER)) executerCommande = true; 

        BeginDrawing();
        ClearBackground(BLACK); 

        BeginScissorMode(0, 0, 750, 495);
        for (int i = 0; i < MAX_HISTORY; i++) {
            if (history[i][0] != '\0') {
                Vector2 pos = { 25.0f, 25.0f + (i * 36) };
                Color textColor = (history[i][0] == '>') ? LIGHTGRAY : WHITE;
                DrawTextEx(cleanMonoFont, history[i], pos, 20.0f, 1.0f, textColor);
            }
        }
        EndScissorMode();

        DrawLine(0, 510, 800, 510, DARKGRAY);
        
        GuiTextBox((Rectangle){ 25, 530, 605, 40 }, textBuffer, BUFFER_SIZE, true);
        
        if (GuiButton((Rectangle){ 645, 530, 130, 40 }, "Envoyer")) executerCommande = true;

        EndDrawing();

        if (executerCommande && textBuffer[0] != '\0') {
            
            // Log local de la commande tapée
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

    UnloadFont(cleanMonoFont);
    CloseWindow();
    close(fd);
    return 0;
}