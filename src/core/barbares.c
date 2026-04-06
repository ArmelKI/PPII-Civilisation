#include <stdlib.h>
#include <stdio.h>
#include <math.h>

#include "core/barbares.h"
#include "core/types.h"

// Place les campements barbares au démarrage
BarbarianCamp *barbares_init_camps(Map *map, int num_camps, int player_city_x, int player_city_y)
{
    if (num_camps <= 0 || !map) {
        return NULL;
    }

    BarbarianCamp *camps = (BarbarianCamp *)malloc(num_camps * sizeof(BarbarianCamp));
    if (!camps) {
        fprintf(stderr, "[barbares] Erreur allocation campements\n");
        return NULL;
    }

    for (int i = 0; i < num_camps; ++i) {
        camps[i].id = i;
        camps[i].destroyed = 1;
        camps[i].x = -1;
        camps[i].y = -1;
    }

    for (int i = 0; i < num_camps; ++i) {
        int placed = 0;

        for (int attempt = 0; attempt < 500 && !placed; ++attempt) {
            int x = rand() % map->width;
            int y = rand() % map->height;

            if (map->tiles[y][x].type == TERRAIN_EAU) {
                continue;
            }

            if (map->tiles[y][x].city || map->tiles[y][x].unit) {
                continue;
            }

            if (abs(x - player_city_x) < 5 && abs(y - player_city_y) < 5) {
                continue;
            }

            camps[i].x = x;
            camps[i].y = y;
            camps[i].destroyed = 0;
            placed = 1;
        }

        if (!placed) {
            for (int y = 0; y < map->height && !placed; ++y) {
                for (int x = 0; x < map->width && !placed; ++x) {
                    if (map->tiles[y][x].type == TERRAIN_EAU) {
                        continue;
                    }

                    if (map->tiles[y][x].city || map->tiles[y][x].unit) {
                        continue;
                    }

                    if (abs(x - player_city_x) < 5 && abs(y - player_city_y) < 5) {
                        continue;
                    }

                    camps[i].x = x;
                    camps[i].y = y;
                    camps[i].destroyed = 0;
                    placed = 1;
                }
            }
        }

        if (!placed) {
            free(camps);
            return NULL;
        }
    }

    return camps;
}

// Génère une unité barbare chaque tour (plafond N*3)
int barbares_spawn_unit(BarbarianCamp *camp, Map *map, Unit **units_array, int *num_units, int max_units)
{
    // TODO: créer guerrier barbare si camp actif et tuile libre

    (void)max_units;

    if (!camp || camp->destroyed || !map || !units_array || !num_units) {
        return 0;
    }

    fprintf(stderr, "[TODO] spawn_unit campement %d\n", camp->id);
    return 0;
}

// Pathfinding simple vers cible la plus proche
void barbares_move(Unit *barb, Map *map, City *player_cities[], int num_cities, 
                   Unit *player_units[], int num_player_units)
{
    // TODO: trouver cible la plus proche (distance Tchebychev)
    // TODO: se déplacer d'une tuile vers la cible

    (void)num_cities;
    (void)num_player_units;
    (void)player_cities;
    (void)player_units;

    if (!barb || !map) {
        return;
    }

    fprintf(stderr, "[TODO] move barbare en (%d,%d)\n", barb->x, barb->y);
}

// Combat automatique si adjacent
int barbares_attack_if_adjacent(Unit *barb, Map *map, City *player_cities[], int num_cities,
                                 Unit *player_units[], int num_player_units)
{
    // TODO: chercher cible à distance 1 et attaquer

    (void)num_cities;
    (void)num_player_units;
    (void)player_cities;
    (void)player_units;

    if (!barb || !map) {
        return 0;
    }

    fprintf(stderr, "[TODO] attack barbare en (%d,%d)\n", barb->x, barb->y);
    return 0;
}

// Détruit un campement et donne bonus or
void barbares_destroy_camp(BarbarianCamp *camp, int *total_destroyed, Player *player)
{
    // TODO: marquer destroyed, incrémenter compteur, bonus or au joueur

    (void)total_destroyed;
    (void)player;

    if (!camp || camp->destroyed) {
        return;
    }

    fprintf(stderr, "[TODO] destroy camp %d\n", camp->id);
}

// Libère la mémoire des campements
void barbares_cleanup(BarbarianCamp *camps, int num_camps)
{
    (void)num_camps;

    if (camps) {
        free(camps);
    }
}
