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
        fprintf(stderr, "[barbares] Erreur allocation campements\n");
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
        // Ici il manque encore le vrai déplacement.
        // L'idée sera de chercher la cible la plus proche puis d'avancer d'une case.

    (void)num_cities;
    (void)num_player_units;
    (void)player_cities;
    (void)player_units;

    if (!barb || !map) {
        return;
    }

    // Message temporaire pour garder une trace pendant le développement.
    fprintf(stderr, "[TODO] move barbare en (%d,%d)\n", barb->x, barb->y);
}

// Combat automatique si adjacent
int barbares_attack_if_adjacent(Unit *barb, Map *map, City *player_cities[], int num_cities,
                                 Unit *player_units[], int num_player_units)
{
    // Même principe que pour le déplacement : version simple pour l'instant.

    (void)num_cities;
    (void)num_player_units;
    (void)player_cities;
    (void)player_units;

    if (!barb || !map) {
        return 0;
    }

    // Le vrai combat sera ajouté plus tard.
    fprintf(stderr, "[TODO] attack barbare en (%d,%d)\n", barb->x, barb->y);
    return 0;
}

// Détruit un campement et donne bonus or
void barbares_destroy_camp(BarbarianCamp *camp, int *total_destroyed, Player *player)
{
    // La logique exacte sera reliée au joueur quand la gestion du combat sera prête.

    (void)total_destroyed;
    (void)player;

    if (!camp || camp->destroyed) {
        return;
    }

    // Placeholder pour l'instant.
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
