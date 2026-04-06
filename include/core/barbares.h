#ifndef CORE_BARBARES_H
#define CORE_BARBARES_H

#include "types.h"

/*
 * Module des Barbares - Gestion des campements et unités barbares.
 * 
 * Responsabilités :
 * - Placement initial des campements (aléatoire, contraintes de distance)
 * - Génération d'unités barbares chaque tour (plafond N×3)
 * - Pathfinding des barbares (vers cible la plus proche)
 * - Attaque automatique des barbares adjacents
 * - Destruction des campements quand frappés par un joueur
 * 
 * À implémenter progressivement.
 */

/*
 * Initialise N campements barbares aléatoirement sur la carte.
 * Contraintes : 
 *   - Au moins 5 tuiles (distance Tchebychev) de la ville de départ du joueur
 *   - Pas sur une tuile d'eau
 *   - Pas sur une tuile occupée
 * 
 * Retourne : tableau de BarbarianCamp alloué dynamiquement (à libérer par caller)
 */
BarbarianCamp *barbares_init_camps(Map *map, int num_camps, int player_city_x, int player_city_y);

/*
 * Génère une unité barbare à partir d'un campement s'il est actif et sa tuile est libre.
 * À appeler pour chaque campement chaque tour.
 * 
 * Retourne : 1 si unité générée, 0 sinon.
 */
int barbares_spawn_unit(BarbarianCamp *camp, Map *map, Unit **units_array, int *num_units, int max_units);

/*
 * Déplace une unité barbare vers la cible (unité ou ville du joueur) la plus proche.
 * Utilise pathfinding simple : approche via distance Tchebychev.
 * 
 * Retourne : nouvelle position (x, y) mise à jour dans l'unité, ou (-1, -1) si pas de mouvement.
 */
void barbares_move(Unit *barb, Map *map, City *player_cities[], int num_cities, 
                   Unit *player_units[], int num_player_units);

/*
 * Vérifie si une unité barbare est adjacente (distance 1) à une cible du joueur.
 * Si oui, attaque automatiquement.
 * 
 * Retourne : 1 si combat engagé, 0 sinon.
 */
int barbares_attack_if_adjacent(Unit *barb, Map *map, City *player_cities[], int num_cities,
                                Unit *player_units[], int num_player_units);

/*
 * Détruit un campement barbare quand le joueur marche dessus.
 * Bonus au joueur : or = 5 × (nombre de campements détruits jusque-là, inclus)
 */
void barbares_destroy_camp(BarbarianCamp *camp, int *total_destroyed, Player *player);

/*
 * Nettoie les ressources allouées pour les barbares.
 */
void barbares_cleanup(BarbarianCamp *camps, int num_camps);

#endif // CORE_BARBARES_H
