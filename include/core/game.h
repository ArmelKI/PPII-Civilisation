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

    // Barbares: tableau de campements initialisés à la création de la partie.
    // Invariants et usage:
    // - `barbarian_camp_count` indique le nombre de campements logiques (N) créés au démarrage.
    // - `barbarian_camps_destroyed` compte les campements rasés ; utilisé pour calculer
    //   le bonus d'or cumulatif accordé au joueur (5 × camps_détruits).
    // - Chaque case de la carte qui contient un camp pointe vers l'entrée correspondante
    //   via `map.tiles[y][x].camp` (on n'enregistre jamais d'adresses volatiles dans les fichiers).
    BarbarianCamp *barbarian_camps;
    int barbarian_camp_count;
    int barbarian_camps_destroyed;

    int turn;
    int max_turns;
    int consecutive_turns_10_cities;
    int consecutive_zero_production_turns;
    int game_over;
    int seed;

    int wonder_built[NUM_WONDERS];       // 1 si construite, 0 sinon
    int wonder_x[NUM_WONDERS];           // coordonnées de la merveille (si built)
    int wonder_y[NUM_WONDERS];
    int wonder_host_city_id[NUM_WONDERS];// id de la ville qui héberge (pour gérer destruction)
} GameState;

// --- FONCTIONS DE GESTION DU JEU ---

// Initialisation globale
void init_game(GameState *game, int width, int height, int seed, int barbarian_camp_count);

// Logique de tour
void next_turn(GameState *game);
void check_victory_and_defeat(GameState *game);
int calculate_final_score(GameState *game);

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