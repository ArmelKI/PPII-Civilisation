#ifndef CORE_BARBARES_H
#define CORE_BARBARES_H

#include <stddef.h>
#include "types.h"

// Fonctions liées aux barbares : campements, unités et combats.

// Place les campements barbares au début de la partie.
// Le tableau retourné est alloué dynamiquement.
BarbarianCamp *barbares_init_camps(Map *map, int num_camps, int player_city_x, int player_city_y);

// Crée un barbare à partir d'un campement actif si la case est libre.
// Retourne 1 si un barbare est créé, 0 sinon.
int barbares_spawn_unit(BarbarianCamp *camp, Map *map, Unit **units_array, int *num_units, int max_units);

// Déplace un barbare vers la cible la plus proche.
// En cas d'égalité, l'ordre d'apparition des cibles dans les tableaux
// (unités puis villes) sert de départage déterministe.
void barbares_move(Unit *barb, Map *map, City *player_cities[], int num_cities, 
                   Unit *player_units[], int num_player_units);

// Vérifie si le barbare peut attaquer automatiquement une cible voisine.
// Retourne 1 si un combat a eu lieu, 0 sinon.
// Si `out_log` est fourni, la fonction y écrit un résumé lisible du combat.
int barbares_attack_if_adjacent(Unit *barb, Map *map, City *player_cities[], int num_cities,
                                Unit *player_units[], int num_player_units,
                                char *out_log, size_t out_log_len);

// Détruit un campement et donne le bonus d'or associé.
void barbares_destroy_camp(BarbarianCamp *camp, int *total_destroyed, Player *player);

// Libère la mémoire du tableau de campements.
void barbares_cleanup(BarbarianCamp *camps, int num_camps);

#endif // CORE_BARBARES_H
