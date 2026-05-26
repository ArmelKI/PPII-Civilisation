#ifndef CORE_BARBARES_H
#define CORE_BARBARES_H

#include "types.h"

// Fonctions liées aux barbares : campements, unités et combats.

// Place les campements barbares au début de la partie.
// Le tableau retourné est alloué dynamiquement.
BarbarianCamp *barbares_init_camps(Map *map, int num_camps, int player_city_x, int player_city_y);

// Crée un barbare à partir d'un campement actif si la case est libre.
// Retourne 1 si un barbare est créé, 0 sinon.
int barbares_spawn_unit(BarbarianCamp *camp, Map *map, Unit **units_array, int *num_units, int max_units);

// Déplace un barbare vers la cible la plus proche.
// Ici on garde une logique simple, sans vrai A* pour l'instant.
void barbares_move(Unit *barb, Map *map, City *player_cities[], int num_cities, 
                   Unit *player_units[], int num_player_units);

// Vérifie si le barbare peut attaquer automatiquement une cible voisine.
// Retourne 1 si un combat a eu lieu, 0 sinon.
int barbares_attack_if_adjacent(Unit *barb, Map *map, City *player_cities[], int num_cities,
                                Unit *player_units[], int num_player_units);

// Détruit un campement et applique le bonus d'or: 5 * camps détruits (cumul joueur).
void barbares_destroy_camp(BarbarianCamp *camp, Player *player);

// Libère la mémoire du tableau de campements.
void barbares_cleanup(BarbarianCamp *camps, int num_camps);

#endif // CORE_BARBARES_H
