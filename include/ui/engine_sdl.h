#ifndef ENGINE_SDL_H
#define ENGINE_SDL_H

#include <stdbool.h>
#include "core/types.h"
#include "core/game.h"

// Taille en pixels de chaque côté d'une tuile carrée
#define TILE_SIZE 64

// Fonction principale qui gère la fenêtre et la boucle de jeu SDL
void run_sdl_mode(GameState *game, int nb_turns, bool enable_fow_ui, int* city_id_counter);

#endif // ENGINE_SDL_H