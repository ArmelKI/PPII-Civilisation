# Makefile du projet CIV
# But : compiler simplement le jeu et garder un fichier lisible pour l'equipe.

CC := gcc

PROJECT := civ
SRC_DIR := src
TEST_DIR := tests
INCLUDE_DIR := include
BUILD_DIR := build
BIN_DIR := bin

# Options de compilation pour le code C
# - les warnings sont actifs
# - les fichiers .d sont generes pour suivre les dependances
CFLAGS := -std=c11 -Wall -Wextra -Wpedantic -Werror -MMD -MP
CPPFLAGS := -I$(INCLUDE_DIR)
LDFLAGS =
LDLIBS := -lncursesw

# Nom des executables produits par le projet
APP_BIN := $(BIN_DIR)/$(PROJECT)
TEST_BIN := $(BIN_DIR)/tests_runner

# On recupere automatiquement tous les .c du projet et des tests
APP_SRC := $(shell find $(SRC_DIR) -type f -name '*.c' 2>/dev/null)
TEST_SRC := $(shell find $(TEST_DIR) -type f -name '*.c' 2>/dev/null)

# Flags SDL
SDL_CFLAGS := $(shell pkg-config --cflags sdl2 SDL2_ttf SDL2_image)
SDL_LIBS   := $(shell pkg-config --libs sdl2 SDL2_ttf SDL2_image)

# Compilation conditionnelle -> on compile les fichiers SDL seulement si on utilise cette version
ifeq ($(filter sdl,$(MAKECMDGOALS)),sdl)
    # Vérification que SDL_LIBS n'est pas vide
    ifeq ($(strip $(SDL_LIBS)),)
        $(error "ERREUR: SDL2, SDL2_ttf ou SDL2_image introuvables via pkg-config. Installez les paquets -dev.")
    endif
    
    CFLAGS += -DUSE_SDL
    CPPFLAGS += $(SDL_CFLAGS)
    # On ajoute les libs SDL ET ncurses
    LDLIBS := $(SDL_LIBS) -lncursesw
else    
    # Si pas de sdl, on exclut le fichier de la compilation
    APP_SRC := $(filter-out %engine_sdl.c, $(APP_SRC))
    LDLIBS := -lncursesw
endif

# Transformation des fichiers source en objets
APP_OBJ := $(patsubst %.c,$(BUILD_DIR)/%.o,$(APP_SRC))
TEST_OBJ := $(patsubst %.c,$(BUILD_DIR)/%.o,$(TEST_SRC))
# On exclut main.c pour le binaire de tests, si on veut tester le reste du moteur
CORE_SRC_NO_MAIN := $(filter-out $(SRC_DIR)/core/main.c,$(APP_SRC))
CORE_OBJ_NO_MAIN := $(patsubst %.c,$(BUILD_DIR)/%.o,$(CORE_SRC_NO_MAIN))

# Fichiers de dependances generes automatiquement par le compilateur
APP_DEP := $(APP_OBJ:.o=.d)
TEST_DEP := $(TEST_OBJ:.o=.d) $(CORE_OBJ_NO_MAIN:.o=.d)

.PHONY: all req cli sdl test clean reset help

# Cible par defaut : compiler le jeu
all: $(APP_BIN)

# Petit aide-memoire pour les commandes principales
help:
	@echo "Cibles disponibles :"
	@echo "  make        : compile le projet"
	@echo "  make req    : verifie les dependances"
	@echo "  make cli    : lance en mode CLI"
	@echo "  make sdl    : lance en mode SDL"
	@echo "  make test   : lance les tests"
	@echo "  make clean  : supprime les artefacts de build"
	@echo "  make reset  : supprime les donnees generees"

# Liaison du binaire principal
$(APP_BIN): $(APP_OBJ)
	@mkdir -p $(dir $@)
	$(CC) $(LDFLAGS) $^ -o $@ $(LDLIBS)

# Regle generique : un .c devient un .o
$(BUILD_DIR)/%.o: %.c
	@mkdir -p $(dir $@)
	$(CC) $(CPPFLAGS) $(CFLAGS) -c $< -o $@

# Lancement de la version terminal
cli: $(APP_BIN)
	./$(APP_BIN) --mode cli

# Lancement de la version SDL
sdl: $(APP_BIN)
	./$(APP_BIN) --mode sdl

# Les tests sont facultatifs au debut du projet
test:
	@if [ -z "$(strip $(TEST_SRC))" ]; then \
		echo "Aucun test detecte dans $(TEST_DIR)."; \
	else \
		$(MAKE) --no-print-directory $(TEST_BIN); \
		./$(TEST_BIN); \
	fi

# Binaire de tests lie avec le reste du moteur, sans main.c
$(TEST_BIN): $(TEST_OBJ) $(CORE_OBJ_NO_MAIN)
	@mkdir -p $(dir $@)
	$(CC) $(LDFLAGS) $^ -o $@ $(LDLIBS)

# Verification rapide des outils installes sur la machine
req:
	@echo "Verification des prerequis..."
	@command -v $(CC) >/dev/null 2>&1 || { echo "Erreur: $(CC) introuvable"; exit 1; }
	@if command -v pkg-config >/dev/null 2>&1 && pkg-config --exists sdl2 SDL2_ttf; then \
		echo "SDL2 et SDL2_ttf detecte via pkg-config."; \
	else \
		echo "SDL2 ou SDL2_ttf non detecte automatiquement."; \
		echo "Installez SDL2 (ex: sudo apt install libsdl2-dev libsdl2-ttf-dev)."; \
	fi

# Nettoyage des fichiers de compilation et des binaires
clean:
	rm -rf $(BUILD_DIR) $(BIN_DIR) current_game.tmp

# Nettoyage complet : compilation + donnees generees
reset: clean
	rm -rf data/saves data/config data/scores

# Les dependances sont chargees seulement si elles existent deja
-include $(APP_DEP)
-include $(TEST_DEP)
