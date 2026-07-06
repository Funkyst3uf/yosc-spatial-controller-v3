/*
 * Fichier : main_gui.c
 * Point d'entrée du programme de contrôle du processeur immersif Yamaha DME7
 * Auteur : Jonathan Ntoula - Juin 2026
 * Pilote le cycle de vie du programme avec interface.
 * Projet réalisé pour le cours Interprétation & Compilation / Logiciels Libres
 * dirigé par M. Kislin - Licence Informatique, Université Paris 8
 */

#include "sys.h"
#include "dial.h"
#include "spatial.h"
#include "audio.h"
#include "y.tab.h" /* Généré par Bison, contient les définitions de tokens */

// Inclusion de la bibliothèque de base (fenetre, entrees, graphismes bas niveau)
#include "raylib.h"
// Cette macro active le vrai code source des boutons et widgets de Raygui avant inclusion
#define RAYGUI_IMPLEMENTATION
#include "raygui.h"

#include <string.h>
#include <stdio.h>
#include <stdbool.h>
#include <stdlib.h>

#define MAX_HISTORY 13  
#define BUFFER_SIZE 256

extern Point3D objets[];

// Descripteur fichier (socket UDP) utilisé pour la communication UDP avec le processeur Yamaha DME7
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

    // masque les messages de démarrage internes de Raylib pour ne garder que les erreurs graves
    SetTraceLogLevel(LOG_ERROR);
    printf("Démarrage du pilote DME7 (Version avec UI)...\n");


    // appel de la fonction dial() avec les informations saisies par l'utilisateur
    fd = dial(av[1], av[2]);
    if (fd < 0) { // en cas d'échec de connexion
        perror("Erreur de connexion (dial)");
        return 1;
    }

    /* Délégation de l'initialisation des tables (objets et labels) au module spatial.c */
    init_tables(); 

    // Initialise et ouvre la fenetre graphique avec sa taille et son titre
    InitWindow(800, 600, "Terminal de controle DME7");
    // Bloque la vitesse de rafraichissement de la boucle à 60 images par seconde
    SetTargetFPS(60);

    /* Préparation de la table ASCII (0 à 255)
       Génère la liste des identifiants de caractères à extraire du fichier TTF.
       Permet à Raylib de charger en mémoire l'alphabet complet, les chiffres, 
       les symboles du terminal et l'ensemble des accents français (é, à, ç, è). */
    int codepoints[256];
    for (int i = 0; i < 256; i++) codepoints[i] = i;

    // Charge en mémoire la police de caractère personnalisée pour le style rétro/terminal
    Font cleanMonoFont = LoadFontEx("assets/fonts/UbuntuMono-R.ttf", 24, codepoints, 256);
    
    // Associe la police chargée à tous les futurs composants graphiques de Raygui
    GuiSetFont(cleanMonoFont);

    // Règle la taille du texte par défaut pour l'interface graphique
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
    strcpy(history[MAX_HISTORY - 1], "Système connecté au DME7. Prêt à envoyer des commandes.");

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
                Vector2 pos = { 25.0f, 25.0f + (i * 36) };
                Color textColor = (history[i][0] == '>') ? LIGHTGRAY : WHITE;
                // Dessine une ligne de texte de l'historique avec la police personnalisée
                DrawTextEx(cleanMonoFont, history[i], pos, 20.0f, 1.0f, textColor);
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
        // On boucle sur tout l'historique sauf le dernier élément.
        for (int i = 0; i < (MAX_HISTORY - 1); i++) {
            // memmove déplace le contenu de la case i+1 vers la case i.
            // l'ancienne commande est écrasée et toutes les autres montent d'un cran.
            memmove(history[i], history[i + 1], BUFFER_SIZE + 8);
        }
        
        // Ajout de la nouvelle commande à la fin de l'historique
        // On copie le texte tapé dans la toute dernière case libérée (MAX_HISTORY - 1)
        // en rajoutant le préfixe "> " pour simuler un prompt de terminal.
        snprintf(history[MAX_HISTORY - 1], BUFFER_SIZE + 8, "> %s", textBuffer);

        // Détection de la commande de sortie
        // On compare le texte saisi avec le mot clé "QUIT". Si c'est identique, doitQuitter devient vrai (true).
        bool doitQuitter = (strcmp(textBuffer, "QUIT") == 0);
        
        // Préparation du flux pour le parseur (Mise au format)
        char tempBuffer[BUFFER_SIZE + 2];
        // copie la commande et ajoute "\n" à la fin.
        snprintf(tempBuffer, sizeof(tempBuffer), "%s\n", textBuffer);

        // Initialisation des pointeurs système pour la RAM
        // ptrBuffer pointera vers l'adresse en mémoire où le texte sera récupéré,
        // et sizeBuffer stockera dynamiquement la taille de ce texte.
        char *ptrBuffer = NULL;
        size_t sizeBuffer = 0;
            
            // Ouvre le fichier virtuel en RAM pour intercepter les flux
            FILE *memStream = open_memstream(&ptrBuffer, &sizeBuffer);

            FILE *old_stdout = stdout;
            FILE *old_stderr = stderr;

            // Redirection des sorties textuelles du système vers la RAM
            stdout = memStream;
            stderr = memStream;

            // Branchement de la chaîne sur le Lexer (Flex)
            // On dit à Flex de ne pas lire l'entrée standard (clavier), mais d'analyser le contenu de "tempBuffer".
            // yy_scan_string crée un tampon (buffer) virtuel en RAM à partir de cette chaîne.
            YY_BUFFER_STATE bp = yy_scan_string(tempBuffer);
            
            // Lancement de l'analyse syntaxique (Bison)
            // En mode UI, on ignore le code de retour (0 ou 1) de la fonction yyparse()
            // si une erreur survient, yyerror() écrit directement le message dans lr 'memStream' en RAM, 
            // ce qui permet à l'UI de l'afficher à l'écran sans interrompre l'application.
            yyparse();
            
            // Libération de la mémoire de Flex
            // L'analyse est finie, on détruit proprement le tampon virtuel "bp" pour éviter les fuites de mémoire.
            yy_delete_buffer(bp);

            // Actualisation du flux virtuel (RAM)
            // On force l'écriture de tout ce que le parseur a pu générer (logs, messages) dans le memStream.
            // C'est cette étape qui met à jour "ptrBuffer" et sa taille "sizeBuffer".
            fflush(memStream);
            
            // Fermeture du flux
            // On ferme proprement le fichier virtuel en RAM. 
            // À partir d'ici, "ptrBuffer" contient la chaîne de caractères finale que ton UI va pouvoir afficher.
            fclose(memStream);
            
            // Restauration des sorties standards vers leur état normal
            stdout = old_stdout;
            stderr = old_stderr;

            // Vérification de la présence de données dans le flux virtuel
            // Si le pointeur n'est pas nul et que la taille est supérieure à 0, il y a des logs à traiter.
            if (ptrBuffer != NULL && sizeBuffer > 0) {
                
                // Découpage du texte récupéré ligne par ligne
                // strtok utilise les caractères de retour à la ligne (\n ou \r) comme séparateurs.
                char *line = strtok(ptrBuffer, "\n\r");
                
                // Boucle de traitement pour chaque ligne extraite
                while (line != NULL) {
                    
                    // On ignore les lignes complètement vides
                    if (strlen(line) > 0) {
                        
                        // Décalage de l'historique vers le haut pour faire de la place
                        // La ligne la plus ancienne est écrasée à chaque rotation.
                        for (int i = 0; i < (MAX_HISTORY - 1); i++) {
                            memmove(history[i], history[i + 1], BUFFER_SIZE + 8);
                        }
                        
                        // Copie de la ligne de log actuelle dans la dernière case de l'historique
                        snprintf(history[MAX_HISTORY - 1], BUFFER_SIZE + 8, "%s", line);
                    }
                    
                    // Passage à la ligne suivante du texte (NULL indique à strtok de continuer sur la même chaîne)
                    line = strtok(NULL, "\n\r");
                }
                
                // Libération de la mémoire vive
                free(ptrBuffer); 
            }

            // Réinitialisation complète du champ de saisie de l'UI
            // On place le caractère de fin de chaîne au début pour vider le texte affiché dans la boîte.
            textBuffer[0] = '\0';
            
            // Sortie de la boucle principale de l'application si la commande était "QUIT"
            if (doitQuitter) break;
        }
    }

    // Libère la mémoire occupée par la police de caractères
    UnloadFont(cleanMonoFont);

    // Ferme proprement la fenetre graphique
    CloseWindow();

    /* Fermeture propre de la ressource réseau */
    close(fd);
    
    return 0;
}