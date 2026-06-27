# Makefile pour le contrôleur spatial DME7
# Auteur : Jonathan Ntoula

CC = gcc
YACC = yacc
LEX = lex
CFLAGS = -Wall -Iinclude
LIBS = -llo -lraylib -lGL -lm -lpthread -ldl -lrt -lX11

CLIENT_TARGET = spatial-controller
SERVER_TARGET = dme7-sim

# --- DOSSIER DES OBJETS ---
OBJ_DIR = obj

# Liste des objets avec le préfixe du dossier obj/
CLIENT_OBJ = $(addprefix $(OBJ_DIR)/, main_gui.o dial.o spatial.o audio.o y.tab.o lex.yy.o)
SERVER_OBJ = $(OBJ_DIR)/dme7_simulator.o

HEADERS = include/sys.h include/dial.h include/spatial.h include/audio.h src/y.tab.h

all: $(CLIENT_TARGET) $(SERVER_TARGET)

client: $(CLIENT_TARGET)
server: $(SERVER_TARGET)
parser: src/y.tab.c src/y.tab.h

tomato: src/yosc.y
	cd src && $(YACC) -v yosc.y

# --- ÉDITION DE LIENS ---

$(CLIENT_TARGET): $(CLIENT_OBJ)
	$(CC) $(CLIENT_OBJ) -o $(CLIENT_TARGET) $(LIBS)

$(SERVER_TARGET): $(SERVER_OBJ)
	$(CC) $(SERVER_OBJ) -o $(SERVER_TARGET) -llo

# --- GÉNÉRATION DES SOURCES DE LA GRAMMAIRE ---

src/y.tab.c src/y.tab.h: src/yosc.y
	cd src && $(YACC) -d yosc.y

src/lex.yy.c: src/yosc.l src/y.tab.h
	cd src && $(LEX) yosc.l

# --- COMPILATION DES FICHIERS OBJETS (avec création auto de obj/) ---

$(OBJ_DIR)/main_gui.o: src/main_gui.c $(HEADERS)
	@mkdir -p $(OBJ_DIR)
	$(CC) $(CFLAGS) -c src/main_gui.c -o $@

$(OBJ_DIR)/spatial.o: src/spatial.c $(HEADERS)
	@mkdir -p $(OBJ_DIR)
	$(CC) $(CFLAGS) -c src/spatial.c -o $@

$(OBJ_DIR)/dial.o: src/dial.c $(HEADERS)
	@mkdir -p $(OBJ_DIR)
	$(CC) $(CFLAGS) -c src/dial.c -o $@

$(OBJ_DIR)/audio.o: src/audio.c $(HEADERS)
	@mkdir -p $(OBJ_DIR)
	$(CC) $(CFLAGS) -c src/audio.c -o $@

$(OBJ_DIR)/dme7_simulator.o: src/dme7_simulator.c include/sys.h
	@mkdir -p $(OBJ_DIR)
	$(CC) $(CFLAGS) -c src/dme7_simulator.c -o $@

$(OBJ_DIR)/y.tab.o: src/y.tab.c src/y.tab.h
	@mkdir -p $(OBJ_DIR)
	$(CC) $(CFLAGS) -c src/y.tab.c -o $@

$(OBJ_DIR)/lex.yy.o: src/lex.yy.c src/y.tab.h
	@mkdir -p $(OBJ_DIR)
	$(CC) $(CFLAGS) -c src/lex.yy.c -o $@

# --- NETTOYAGE ---

clean:
	rm -rf $(OBJ_DIR) $(CLIENT_TARGET) $(SERVER_TARGET) src/y.tab.c src/y.tab.h src/y.output src/lex.yy.c

re: clean all

.PHONY: all clean re client server parser tomato
