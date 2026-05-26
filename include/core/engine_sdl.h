#ifndef ENGINE_SDL_H
#define ENGINE_SDL_H

#include <stdbool.h>
#include "types.h"
#include "game.h"

// Taille en pixels de chaque côté d'une tuile carrée
#define TILE_SIZE 64

// Fonction principale qui gère la fenêtre et la boucle de jeu SDL
void run_sdl_mode(GameState *game, int nb_turns, bool enable_fow_ui);

#endif // ENGINE_SDL_H