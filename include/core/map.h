#ifndef CORE_MAP_H
#define CORE_MAP_H

#include "types.h"

// ----------------Fonctions de bases---------------------

// Alloue et génère la carte
Map* create_map(int width, int height,Player* player, int seed);
Map* create_map_noise(int width, int height,Player* player, int seed);

// Affiche la carte dans le terminal (TUI)
void display_map(Map* map);
void display_map_color(Map* map);

// Libère la mémoire
void free_map(Map* map);

// ----------------Fonctions utilitaires---------------------

// Place une unité sur une case spécifique
void set_unit_at(Map* map, int x, int y, Unit* unit);

#endif