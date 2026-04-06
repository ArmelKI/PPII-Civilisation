#include <stdlib.h>
#include <stdio.h>
#include <math.h>

#include "core/barbares.h"
#include "core/types.h"

/*
 * Module d'implémentation des Barbares.
 * À compléter progressivement.
 */

/* ============================================================================
   PLACEMENT INITIAL DES CAMPEMENTS
   ============================================================================ */

BarbarianCamp *barbares_init_camps(Map *map, int num_camps, int player_city_x, int player_city_y)
{
    /*
     * TODO : Implémenter
     * 
     * Algorithme simplifié :
     * 1. Allouer tableau de BarbarianCamp[num_camps]
     * 2. Pour chaque campement à placer :
     *    a. Générer (x, y) aléatoire
     *    b. Vérifier contraintes :
     *       - Distance >= 5 (Tchebychev) du joueur
     *       - Pas d'eau, pas occupé
     *    c. Si OK : placer, sinon réessayer
     * 3. Retourner le tableau alloué
     */

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

    // Placeholder : on laisse 'destroyed = 1' jusqu'à implémentation du placement
    fprintf(stderr, "[barbares] PLACEHOLDER : %d campements à placer (non implémenté)\n", num_camps);

    return camps;
}

/* ============================================================================
   GÉNÉRATION D'UNITÉS PAR TOUR
   ============================================================================ */

int barbares_spawn_unit(BarbarianCamp *camp, Map *map, Unit **units_array, int *num_units, int max_units)
{
    /*
     * TODO : Implémenter
     * 
     * Algorithme :
     * 1. Vérifier que camp->destroyed == 0
     * 2. Vérifier que la tuile du camp est libre (map[camp->y][camp->x].unit == NULL)
     * 3. Si OUI :
     *    a. Créer une nouvelle unité UNIT_BARBARIAN à (camp->x, camp->y)
     *    b. Ajouter au tableau units_array
     *    c. Retourner 1
     * 4. Sinon retourner 0
     */

    (void)max_units;

    if (!camp || camp->destroyed || !map || !units_array || !num_units) {
        return 0;
    }

    // Placeholder
    fprintf(stderr, "[barbares] TODO: spawn_unit à partir du campement %d\n", camp->id);
    return 0;
}

/* ============================================================================
   DÉPLACEMENT (PATHFINDING SIMPLE)
   ============================================================================ */

void barbares_move(Unit *barb, Map *map, City *player_cities[], int num_cities, 
                   Unit *player_units[], int num_player_units)
{
    /*
     * TODO : Implémenter
     * 
     * Algorithme simple :
     * 1. Trouver la cible (unité ou ville) la plus proche
     *    - Utilisez distance de Tchebychev : max(abs(dx), abs(dy))
     *    - Parcourez toutes les unités + toutes les villes du joueur
     *    - Si égalité de distance, appliquer règle de départage (e.g., priorité ville)
     * 
     * 2. Se déplacer d'une tuile vers la cible
     *    - Dir = signe(cible_x - barb->x, cible_y - barb->y)
     *    - Nouvelle tuile = (barb->x + dir_x, barb->y + dir_y)
     *    - Coût terrain : 1 ou 2 ou 3 selon terrain
     *    - Si assez de PM, se déplacer et réduire PM
     *    - Sinon, on reste sur place
     * 
     * 3. Mettre à jour map.tiles
     */

    (void)num_cities;
    (void)num_player_units;
    (void)player_cities;
    (void)player_units;

    if (!barb || !map) {
        return;
    }

    fprintf(stderr, "[barbares] TODO: move barbare en (%d,%d) - pathfinding non implémenté\n", 
            barb->x, barb->y);
}

/* ============================================================================
   ATTAQUE AUTOMATIQUE (SI ADJACENT)
   ============================================================================ */

int barbares_attack_if_adjacent(Unit *barb, Map *map, City *player_cities[], int num_cities,
                                 Unit *player_units[], int num_player_units)
{
    /*
     * TODO : Implémenter
     * 
     * Algorithme :
     * 1. Chercher une cible (unité ou ville) à distance 1 (Tchebychev)
     * 2. Si trouvée :
     *    a. Résoudre combat : 
     *       - Dégâts initiateur = max(1, barb->attack - cible->defense)
     *       - Dégâts retour = max(0, cible->attack - barb->defense)
     *    b. Appliquer dégâts aux deux
     *    c. Si quelqu'un tombe à 0 PV, le retirer de la carte
     *    d. Retourner 1
     * 3. Sinon retourner 0
     */

    (void)num_cities;
    (void)num_player_units;
    (void)player_cities;
    (void)player_units;

    if (!barb || !map) {
        return 0;
    }

    fprintf(stderr, "[barbares] TODO: attack_if_adjacent pour barbare en (%d,%d)\n", 
            barb->x, barb->y);
    return 0;
}

/* ============================================================================
   DESTRUCTION DE CAMPEMENT
   ============================================================================ */

void barbares_destroy_camp(BarbarianCamp *camp, int *total_destroyed, Player *player)
{
    /*
     * TODO : Implémenter
     * 
     * Algorithme :
     * 1. Marquer camp->destroyed = 1
     * 2. Incrémenter total_destroyed
     * 3. Bonus au joueur : player->gold += 5 * total_destroyed
     */

    (void)total_destroyed;
    (void)player;

    if (!camp || camp->destroyed) {
        return;
    }

    fprintf(stderr, "[barbares] TODO: destroy_camp (id=%d)\n", camp->id);
}

/* ============================================================================
   NETTOYAGE
   ============================================================================ */

void barbares_cleanup(BarbarianCamp *camps, int num_camps)
{
    (void)num_camps;

    if (camps) {
        free(camps);
    }
}
