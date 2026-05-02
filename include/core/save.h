#ifndef CORE_SAVE_H
#define CORE_SAVE_H

#include "types.h"
#include "game.h"
// Structure représentant l'état complet du jeu pour la sauvegarde

int save_game(const char*filename, GameState* state);
int load_game(const char*filename, GameState* state);

#endif // CORE_SAVE_H

