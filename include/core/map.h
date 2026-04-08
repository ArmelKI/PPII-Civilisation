#ifndef CORE_MAP_H
#define CORE_MAP_H

#include "types.h"

// Alloue et génère la carte
Map* create_map(int width, int height,Player* player);

// Affiche la carte dans le terminal (TUI)
void display_map(Map* map);
void display_map_color(Map* map);

// Libère la mémoire
void free_map(Map* map);

#endif