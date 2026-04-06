#include <stdlib.h>
#include <stdio.h>
#include <math.h>

#include "core/barbares.h"
#include "core/types.h"

// Place les campements barbares au démarrage
BarbarianCamp *barbares_init_camps(Map *map, int num_camps, int player_city_x, int player_city_y)
{
    // TODO: placement aléatoire avec contrainte distance >= 5 du joueur
    // TODO: vérifier pas d'eau, pas occupé

    (void)player_city_x;
    (void)player_city_y;

    if (num_camps <= 0 || !map) {
        return NULL;
    }

    BarbarianCamp *camps = (BarbarianCamp *)malloc(num_camps * sizeof(BarbarianCamp));
    if (!camps) {
        fprintf(stderr, "[barbares] Erreur allocation campements\n");
        return NULL;
    }

    for (int i = 0; i < num_camps; ++i) {
        camps[i].destroyed = 1;  // Marqué comme détruit pour déterminer quand placer
        camps[i].id = i;
    }

    fprintf(stderr, "[TODO] %d campements barbares à placer\n", num_camps);

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
