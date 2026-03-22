# Makefile d'initialisation du projet CIV
# Objectif: rester simple et lisible pour une equipe etudiante.

CC := gcc

PROJECT := civ
SRC_DIR := src
TEST_DIR := tests
INCLUDE_DIR := include
BUILD_DIR := build
BIN_DIR := bin

# Flags de compilation (stricts mais classiques en C)
CFLAGS := -std=c11 -Wall -Wextra -Wpedantic -Werror -MMD -MP
CPPFLAGS := -I$(INCLUDE_DIR)
LDFLAGS :=
LDLIBS :=

# SDL (utilise pour la cible sdl)
SDL_CFLAGS := $(shell pkg-config --cflags sdl2 2>/dev/null)
SDL_LIBS := $(shell pkg-config --libs sdl2 2>/dev/null)

APP_BIN := $(BIN_DIR)/$(PROJECT)
TEST_BIN := $(BIN_DIR)/tests_runner

APP_SRC := $(shell find $(SRC_DIR) -type f -name '*.c' 2>/dev/null)
TEST_SRC := $(shell find $(TEST_DIR) -type f -name '*.c' 2>/dev/null)

APP_OBJ := $(patsubst %.c,$(BUILD_DIR)/%.o,$(APP_SRC))
TEST_OBJ := $(patsubst %.c,$(BUILD_DIR)/%.o,$(TEST_SRC))
CORE_SRC_NO_MAIN := $(filter-out $(SRC_DIR)/core/main.c,$(APP_SRC))
CORE_OBJ_NO_MAIN := $(patsubst %.c,$(BUILD_DIR)/%.o,$(CORE_SRC_NO_MAIN))

APP_DEP := $(APP_OBJ:.o=.d)
TEST_DEP := $(TEST_OBJ:.o=.d) $(CORE_OBJ_NO_MAIN:.o=.d)

.PHONY: all req cli sdl test clean reset help

# Par défaut: compiler le binaire principal
all: $(APP_BIN)

help:
	@echo "Cibles disponibles :"
	@echo "  make        : compile le projet"
	@echo "  make req    : verifie les dependances"
	@echo "  make cli    : lance en mode CLI"
	@echo "  make sdl    : lance en mode SDL"
	@echo "  make test   : lance les tests"
	@echo "  make clean  : supprime les artefacts de build"
	@echo "  make reset  : supprime les donnees generees"

$(APP_BIN): $(APP_OBJ)
	@mkdir -p $(dir $@)
	$(CC) $(LDFLAGS) $^ -o $@ $(LDLIBS)

# Règle générique de compilation .c -> .o
$(BUILD_DIR)/%.o: %.c
	@mkdir -p $(dir $@)
	$(CC) $(CPPFLAGS) $(CFLAGS) -c $< -o $@

cli: $(APP_BIN)
	./$(APP_BIN) --mode cli

sdl: CPPFLAGS += $(SDL_CFLAGS)
sdl: LDLIBS += $(SDL_LIBS)
sdl: $(APP_BIN)
	./$(APP_BIN) --mode sdl

test:
	@if [ -z "$(strip $(TEST_SRC))" ]; then \
		echo "Aucun test detecte dans $(TEST_DIR)."; \
	else \
		$(MAKE) --no-print-directory $(TEST_BIN); \
		./$(TEST_BIN); \
	fi

$(TEST_BIN): $(TEST_OBJ) $(CORE_OBJ_NO_MAIN)
	@mkdir -p $(dir $@)
	$(CC) $(LDFLAGS) $^ -o $@ $(LDLIBS)

req:
	@echo "Verification des prerequis..."
	@command -v $(CC) >/dev/null 2>&1 || { echo "Erreur: $(CC) introuvable"; exit 1; }
	@if command -v pkg-config >/dev/null 2>&1 && pkg-config --exists sdl2; then \
		echo "SDL2 detecte via pkg-config."; \
	else \
		echo "SDL2 non detecte automatiquement."; \
		echo "Installez SDL2 (ex: sudo apt install libsdl2-dev)."; \
	fi

clean:
	rm -rf $(BUILD_DIR) $(BIN_DIR)

reset: clean
	rm -rf data/saves data/config data/scores

-include $(APP_DEP)
-include $(TEST_DEP)
