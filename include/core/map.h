#ifndef CORE_MAP_H
#define CORE_MAP_H

#include "types.h"
#include "game.h"

// ----------------Fonctions de bases---------------------
typedef struct GameState GameState; // Déclaration anticipée pour éviter les inclusions circulaires
// Alloue et génère la carte
Map* create_map(int width, int height, int seed);
Map* create_map_noise(int width, int height, int seed);

// Affiche la carte dans le terminal (TUI)
void display_map(Map* map);
void display_map_color(Map* map);
void update_fog_of_war(GameState *game); //Logique du brouillard de guerre

// Libère la mémoire
void free_map(Map* map);

// ----------------Fonctions utilitaires---------------------

// Place une unité sur une case spécifique
void set_unit_at(Map* map, int x, int y, Unit* unit);

#endif