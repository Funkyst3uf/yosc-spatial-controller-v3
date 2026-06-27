/**
 * @file yosc.l
 * @brief Analyseur lexical (lexer) pour le contrôleur immersif Yamaha DME7
 * @author Jonathan Ntoula
 * @date Mai 2026
 * @découpe le flux de données saisi par l'utilisateur 
 * en unités textuelles (mots) identifiables via l'utilisation de RegEx
 */


%{   /* Déclaration librairies standards et headers */
#include <stdlib.h>
#include <string.h>
#include "spatial.h"  
#include "y.tab.h"    
%}

/* signale à Flex que nous n'utiliserons pas la fonction input() */
%option noinput  
/* signale à Flex que nous n'utiliserons pas la fonction yyunput() */
%option nounput 

%% /* --- Les règles de l'analyseur --- */

"JUMP"          { return YJUMP; }
"MOVE"          { return YMOVE; }
"MUTE"          { return YMUTE; }
";"             { return YSEMI; }
"SET"           { return YSET; }
"PONG"          { return YPONG; }
"SWAP"          { return YSWAP; }
"STATUS"        { return YSTATUS; }
"QUIT"          { return YQUIT; }
\n              { return YNEWLINE; } 

    /** * @brief Identifiants et labels (chaîne de caractères) */
[a-zA-Z][a-zA-Z0-9]* { yylval.s = strdup(yytext); return YLABEL; }

    /** * @brief Nombres entiers signés (identifiants objet, état de mute, coordonnées ...) */
-?[0-9]+          { yylval.i = atoi(yytext); return YINT; }  

    /** * @brief Nombres décimaux signés (coordonnées, durée de déplacement) */
-?[0-9]+"."[0-9]+ { yylval.f = (float)atof(yytext); return YFLOAT; }

    /** @brief Gestion des blancs (espaces et tabulations). */
[ \t]+          { /* On ignore les espaces et tabulations */ }

    /** @brief Règle par défaut pour capturer les erreurs lexicales. */
.               { printf("Caractère inconnu : %s\n", yytext); }

%%

int yywrap() {
    return 1;
}