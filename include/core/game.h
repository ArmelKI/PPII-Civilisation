#ifndef GAME_H
#define GAME_H

#include "types.h"
#include "map.h"

// Constantes de limites
#define MAX_PLAYER_UNITS 8
#define MAX_BARBARIANS 16
#define MAX_TURNS 200
#define MAX_CITIES 10

// Je regroupe ici tout ce qui doit vivre pendant la partie.
// Ça évite de passer 15 paramètres dans chaque fonction.
typedef struct GameState {
    Map map;
    
    Tile *tile_buffer;

    Player player;
    City *cities[MAX_CITIES];
    int city_count;

    Unit *player_units[MAX_PLAYER_UNITS];
    int player_unit_count;

    Unit *barbarians[MAX_BARBARIANS];
    int barbarian_count;

    int turn;
    int game_over;
    int seed;
} GameState;

// --- FONCTIONS DE GESTION DU JEU ---

// Initialisation globale
void init_game(GameState *game, int width, int height, int seed);

// Logique de tour
void next_turn(GameState *game);

// Gestion des unités
void spawn_random_barbarian(GameState *game);
void move_barbarians_random(GameState *game);
void restore_pm(Unit **units, int count);

// Nettoyage mémoire
void cleanup_game(GameState *game);

void apply_city_income_wassim(Player *player, City *city);

// --- Fonctions externes optionnelles ---
void city_income_wassim(Player *player, City *city) __attribute__((weak));
void display_state_maxence(const Map *map, const Player *player, const City *city,
                           int turn, int player_units, int barbarians) __attribute__((weak));

#endif // GAME_H