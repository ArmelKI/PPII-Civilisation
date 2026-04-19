#include <stdlib.h>
#include <stdio.h>
#include <math.h>

#include "core/barbares.h"
#include "core/types.h"

// Place les campements barbares au démarrage
BarbarianCamp *barbares_init_camps(Map *map, int num_camps, int player_city_x, int player_city_y)
{
    // On commence par vérifier les entrées pour éviter les crashes.
    if (num_camps <= 0 || !map) {
        return NULL;
    }

    BarbarianCamp *camps = (BarbarianCamp *)malloc(num_camps * sizeof(BarbarianCamp));
    if (!camps) {
        fprintf(stderr, "[barbares] Erreur d'allocation des campements\n");
        return NULL;
    }

    for (int i = 0; i < num_camps; ++i) {
        camps[i].id = i;
        camps[i].destroyed = 1;
        camps[i].x = -1;
        camps[i].y = -1;
    }

    // Pour chaque campement, on tente un placement aléatoire.
    // Si ça ne marche pas, on fait un balayage complet de la carte.
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
            // Solution de secours : on parcourt toute la carte.
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
    // On refuse tout de suite si les paramètres ne sont pas cohérents.
    if (!camp || camp->destroyed || !map || !units_array || !num_units) {
        return 0;
    }

    // Le sujet impose un plafond global sur les barbares vivants.
        if (*num_units >= max_units) {
            return 0;
        }

    // Si le camp n'a pas de position valide, on ne tente rien.
        if (camp->x < 0 || camp->y < 0) {
            return 0;
        }

    // Si la case du camp est déjà occupée, on attend le tour suivant.
        if (map->tiles[camp->y][camp->x].unit) {
            return 0;
        }

    // Création d'un guerrier barbare simple.
        Unit *barb = (Unit *)malloc(sizeof(Unit));
        if (!barb) {
            return 0;
        }

        barb->type = UNIT_BARBARIAN;
        barb->hp = 15;
        barb->hp_max = 15;
        barb->attack = 3;
        barb->defense = 2;
        barb->pm = 3;
        barb->pm_max = 3;
        barb->x = camp->x;
        barb->y = camp->y;
        barb->owner = NULL;

        units_array[*num_units] = barb;
        (*num_units)++;
        map->tiles[barb->y][barb->x].unit = barb;

        return 1;
}

// Pathfinding simple vers cible la plus proche
void barbares_move(Unit *barb, Map *map, City *player_cities[], int num_cities, 
                   Unit *player_units[], int num_player_units)
{
    if (!barb || !map || barb->hp <= 0 || barb->pm <= 0) {
        return;
    }

    int target_x = -1;
    int target_y = -1;
    int best_dist = 999999;

    // Cible prioritaire: unités du joueur (ordre du tableau = départage déterministe).
    for (int i = 0; i < num_player_units; ++i) {
        Unit *u = player_units[i];
        if (!u || u->hp <= 0) {
            continue;
        }

        int dx = abs(u->x - barb->x);
        int dy = abs(u->y - barb->y);
        int dist = (dx > dy) ? dx : dy;
        if (dist < best_dist) {
            best_dist = dist;
            target_x = u->x;
            target_y = u->y;
        }
    }

    // Ensuite les villes (uniquement si plus proches, pas en cas d'égalité).
    for (int i = 0; i < num_cities; ++i) {
        City *c = player_cities[i];
        if (!c || c->hp <= 0) {
            continue;
        }

        int dx = abs(c->x - barb->x);
        int dy = abs(c->y - barb->y);
        int dist = (dx > dy) ? dx : dy;
        if (dist < best_dist) {
            best_dist = dist;
            target_x = c->x;
            target_y = c->y;
        }
    }

    if (target_x < 0 || target_y < 0) {
        return;
    }

    int step_x = 0;
    int step_y = 0;

    if (target_x > barb->x) {
        step_x = 1;
    } else if (target_x < barb->x) {
        step_x = -1;
    }

    if (target_y > barb->y) {
        step_y = 1;
    } else if (target_y < barb->y) {
        step_y = -1;
    }

    int new_x = barb->x + step_x;
    int new_y = barb->y + step_y;

    if (new_x < 0 || new_x >= map->width || new_y < 0 || new_y >= map->height) {
        return;
    }

    if (map->tiles[new_y][new_x].type == TERRAIN_EAU) {
        return;
    }

    // On n'empile pas les unités et on ne rentre pas dans une ville.
    if (map->tiles[new_y][new_x].unit || map->tiles[new_y][new_x].city) {
        return;
    }

    int cost = 1;
    if (map->tiles[new_y][new_x].type == TERRAIN_FORET) {
        cost = 2;
    } else if (map->tiles[new_y][new_x].type == TERRAIN_MONTAGNE) {
        cost = 3;
    }

    if (barb->pm < cost) {
        return;
    }

    map->tiles[barb->y][barb->x].unit = NULL;
    barb->x = new_x;
    barb->y = new_y;
    barb->pm -= cost;
    map->tiles[barb->y][barb->x].unit = barb;
}

// Combat automatique si adjacent
int barbares_attack_if_adjacent(Unit *barb, Map *map, City *player_cities[], int num_cities,
                                 Unit *player_units[], int num_player_units)
{
    if (!barb || !map) {
        return 0;
    }

    // Ordre deterministe : on essaie d'abord les unites du joueur, puis les villes.
    for (int i = 0; i < num_player_units; ++i) {
        Unit *target = player_units[i];
        if (!target || target->hp <= 0) {
            continue;
        }

        int dx = abs(target->x - barb->x);
        int dy = abs(target->y - barb->y);
        if (dx > 1 || dy > 1 || (dx == 0 && dy == 0)) {
            continue;
        }

        int dmg_to_target = barb->attack - target->defense;
        if (dmg_to_target < 1) {
            dmg_to_target = 1;
        }

        int dmg_to_barb = target->attack - barb->defense;
        if (dmg_to_barb < 0) {
            dmg_to_barb = 0;
        }

        target->hp -= dmg_to_target;
        barb->hp -= dmg_to_barb;

        if (target->hp <= 0) {
            target->hp = 0;
            if (target->x >= 0 && target->y >= 0) {
                map->tiles[target->y][target->x].unit = NULL;
            }
            player_units[i] = NULL;
        }

        if (barb->hp <= 0) {
            barb->hp = 0;
            if (barb->x >= 0 && barb->y >= 0) {
                map->tiles[barb->y][barb->x].unit = NULL;
            }
        }

        return 1;
    }

    for (int i = 0; i < num_cities; ++i) {
        City *city = player_cities[i];
        if (!city || city->hp <= 0) {
            continue;
        }

        int dx = abs(city->x - barb->x);
        int dy = abs(city->y - barb->y);
        if (dx > 1 || dy > 1 || (dx == 0 && dy == 0)) {
            continue;
        }

        int city_power = city->has_wall ? 30 : 20;
        int dmg_to_city = barb->attack - city_power;
        if (dmg_to_city < 1) {
            dmg_to_city = 1;
        }

        int dmg_to_barb = city_power - barb->defense;
        if (dmg_to_barb < 0) {
            dmg_to_barb = 0;
        }

        city->hp -= dmg_to_city;
        barb->hp -= dmg_to_barb;

        if (city->hp <= 0) {
            city->hp = 0;
        }

        if (barb->hp <= 0) {
            barb->hp = 0;
            if (barb->x >= 0 && barb->y >= 0) {
                map->tiles[barb->y][barb->x].unit = NULL;
            }
        }

        return 1;
    }

    return 0;
}

// Détruit un campement et donne bonus or
void barbares_destroy_camp(BarbarianCamp *camp, int *total_destroyed, Player *player)
{
    if (!camp || camp->destroyed) {
        return;
    }

    camp->destroyed = 1;

    if (total_destroyed) {
        (*total_destroyed)++;
    }

    if (player && total_destroyed) {
        player->gold += 5 * (*total_destroyed);
    }
}

// Libère la mémoire des campements
void barbares_cleanup(BarbarianCamp *camps, int num_camps)
{
    (void)num_camps;

    if (camps) {
        free(camps);
    }
}
