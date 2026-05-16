#ifndef CORE_SAVE_H
#define CORE_SAVE_H

#include "types.h"
#include "game.h"
// Structure représentant l'état complet du jeu pour la sauvegarde

int save_game(const char*filename, GameState* state);
int load_game(const char*filename, GameState* state);
void save_final_score(const char* player_name, int score, int turn, int status);
void display_history_cli();
void display_history_ncurses();

#endif // CORE_SAVE_H

