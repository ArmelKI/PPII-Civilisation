#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "core/barbares.h"
#include "core/map.h"
#include "core/types.h"

/*
 * Fichier de tests unitaires pour le module barbares.
 *
 * But:
 * - valider chaque comportement critique indépendamment
 * - garder des tests simples à lire pour l'équipe
 *
 * Convention:
 * - un test = une fonction
 * - EXPECT(...) incrémente g_failed si une vérification échoue
 */

static int g_failed = 0;

#define EXPECT(cond, msg)                                                       \
    do {                                                                        \
        if (!(cond)) {                                                          \
            fprintf(stderr, "[FAIL] %s\n", (msg));                            \
            g_failed++;                                                         \
        }                                                                       \
    } while (0)

static void init_map(Map *map, int w, int h)
{
    // Petite map de test allouée dynamiquement.
    // On utilise un buffer continu pour simplifier la libération.
    Tile *buffer = (Tile *)calloc((size_t)w * (size_t)h, sizeof(Tile));
    Tile **rows = (Tile **)malloc((size_t)h * sizeof(Tile *));

    if (!buffer || !rows) {
        fprintf(stderr, "Erreur d'allocation de la map de test\n");
        exit(1);
    }

    map->width = w;
    map->height = h;
    map->tiles = rows;

    for (int y = 0; y < h; ++y) {
        rows[y] = &buffer[y * w];
        for (int x = 0; x < w; ++x) {
            rows[y][x].type = TERRAIN_PLAINE;
            rows[y][x].city = NULL;
            rows[y][x].unit = NULL;
        }
    }
}

static void free_test_map(Map *map)
{
    // Libération symétrique de init_map.
    if (!map || !map->tiles) {
        return;
    }

    free(map->tiles[0]);
    free(map->tiles);
    map->tiles = NULL;
}

static void test_init_camps(void)
{
    // Vérifie le placement initial des campements:
    // - pas sur l'eau
    // - pas trop proche de la ville de départ
    // - coordonnées valides
    Map map;
    init_map(&map, 20, 12);

    map.tiles[0][0].type = TERRAIN_EAU;

    City city = {0};
    city.x = 10;
    city.y = 6;
    map.tiles[city.y][city.x].city = &city;

    srand(1234);
    BarbarianCamp *camps = barbares_init_camps(&map, 3, city.x, city.y);

    EXPECT(camps != NULL, "barbares_init_camps retourne NULL");
    if (camps) {
        for (int i = 0; i < 3; ++i) {
                EXPECT(camps[i].destroyed == 0, "campement pas actif après placement");
                EXPECT(camps[i].x >= 0 && camps[i].x < map.width, "x campement invalide");
                EXPECT(camps[i].y >= 0 && camps[i].y < map.height, "y campement invalide");
                EXPECT(map.tiles[camps[i].y][camps[i].x].type != TERRAIN_EAU, "campement placé sur eau");
            EXPECT(!(abs(camps[i].x - city.x) < 5 && abs(camps[i].y - city.y) < 5),
                    "campement trop proche de la ville de départ");
        }
        barbares_cleanup(camps, 3);
    }

    free_test_map(&map);
}

static void test_spawn_unit(void)
{
    // Vérifie la création d'un barbare depuis un camp:
    // - succès nominal
    // - échec si case occupée
    // - échec si camp détruit
    // - échec si plafond atteint
    Map map;
    init_map(&map, 8, 8);

    Unit *barb_slots[4] = {0};
    int num = 0;

    BarbarianCamp camp = {.x = 3, .y = 4, .destroyed = 0, .id = 7};

    int ok = barbares_spawn_unit(&camp, &map, barb_slots, &num, 4);
    EXPECT(ok == 1, "spawn devrait réussir");
    EXPECT(num == 1, "num_units devrait être incrémenté");
    EXPECT(barb_slots[0] != NULL, "slot barbare non rempli");
    EXPECT(map.tiles[4][3].unit == barb_slots[0], "tuile camp non occupée par le barbare");

    ok = barbares_spawn_unit(&camp, &map, barb_slots, &num, 4);
    EXPECT(ok == 0, "spawn doit échouer si tuile camp occupée");

    map.tiles[4][3].unit = NULL;
    camp.destroyed = 1;
    ok = barbares_spawn_unit(&camp, &map, barb_slots, &num, 4);
    EXPECT(ok == 0, "spawn doit échouer si camp détruit");

    camp.destroyed = 0;
    num = 4;
    ok = barbares_spawn_unit(&camp, &map, barb_slots, &num, 4);
    EXPECT(ok == 0, "spawn doit échouer si plafond atteint");

    for (int i = 0; i < 4; ++i) {
        free(barb_slots[i]);
        barb_slots[i] = NULL;
    }

    free_test_map(&map);
}

static void test_move_barbarian(void)
{
    // Vérifie le déplacement:
    // - avance d'une case vers une cible proche
    // - consomme des PM
    // - met à jour correctement l'ancienne et la nouvelle tuile
    // - refus de déplacement sur eau
    Map map;
    init_map(&map, 10, 10);

    Unit barb = {.type = UNIT_BARBARIAN, .hp = 10, .pm = 3, .pm_max = 3, .x = 5, .y = 5};
    map.tiles[5][5].unit = &barb;

    Unit target = {.type = UNIT_WARRIOR, .hp = 10, .x = 7, .y = 5};
    Unit *targets[1] = {&target};

    City *cities[1] = {NULL};

    int old_dist = abs(target.x - barb.x);
    if (abs(target.y - barb.y) > old_dist) {
        old_dist = abs(target.y - barb.y);
    }

    barbares_move(&barb, &map, cities, 0, targets, 1);

    int new_dist = abs(target.x - barb.x);
    if (abs(target.y - barb.y) > new_dist) {
        new_dist = abs(target.y - barb.y);
    }

    EXPECT(new_dist < old_dist, "barbare devrait se rapprocher de sa cible");
    EXPECT(barb.pm == 2, "coût PM incorrect sur plaine");
    EXPECT(map.tiles[barb.y][barb.x].unit == &barb, "nouvelle tuile non mise à jour");
    EXPECT(map.tiles[5][5].unit == NULL, "ancienne tuile non vidée");

    map.tiles[5][6].unit = NULL;
    barb.x = 5;
    barb.y = 5;
    barb.pm = 3;
    map.tiles[5][5].unit = &barb;

    target.x = 5;
    target.y = 7;
    map.tiles[6][5].type = TERRAIN_EAU;

    barbares_move(&barb, &map, cities, 0, targets, 1);
    EXPECT(map.tiles[barb.y][barb.x].type != TERRAIN_EAU, "barbare ne doit pas entrer sur eau");

    free_test_map(&map);
}

static void test_move_barbarian_pathfinding_around_wall(void)
{
    // Vérifie que le barbare contourne un mur d'eau:
    // l'ancienne IA restait bloquée si la case directe était fermée.
    Map map;
    init_map(&map, 10, 10);

    Unit barb = {.type = UNIT_BARBARIAN, .hp = 10, .pm = 3, .pm_max = 3, .x = 2, .y = 5};
    map.tiles[5][2].unit = &barb;

    Unit target = {.type = UNIT_SETTLER, .hp = 10, .hp_max = 10, .x = 8, .y = 5};
    Unit *targets[1] = {&target};
    City *cities[1] = {NULL};

    // Mur vertical qui bloque la trajectoire directe.
    map.tiles[5][3].type = TERRAIN_EAU;
    map.tiles[5][4].type = TERRAIN_EAU;
    map.tiles[5][5].type = TERRAIN_EAU;

    barbares_move(&barb, &map, cities, 0, targets, 1);

    EXPECT(!(barb.x == 3 && barb.y == 5), "pathfinding: ne doit pas tenter la case eau directe");
    EXPECT(map.tiles[barb.y][barb.x].type != TERRAIN_EAU, "pathfinding: ne doit jamais finir sur eau");
    EXPECT(!(barb.x == 2 && barb.y == 5), "pathfinding: doit avancer en contournant le mur");

    free_test_map(&map);
}

static void test_attack_and_destroy(void)
{
    // Vérifie deux comportements:
    // 1) attaque adjacente (PV de l'ennemi et contre-coup)
    // 2) destruction de campement + bonus or joueur
    Map map;
    init_map(&map, 8, 8);

    Unit barb = {.type = UNIT_BARBARIAN, .hp = 10, .attack = 3, .defense = 1, .x = 3, .y = 3};
    map.tiles[3][3].unit = &barb;

    Unit enemy = {.type = UNIT_WARRIOR, .hp = 10, .attack = 2, .defense = 1, .x = 4, .y = 3};
    map.tiles[3][4].unit = &enemy;
    Unit *player_units[1] = {&enemy};
    City *cities[1] = {NULL};

    int fought = barbares_attack_if_adjacent(&barb, &map, cities, 0, player_units, 1);
    EXPECT(fought == 1, "combat adjacent devrait être détecté");
    EXPECT(enemy.hp < 10, "ennemi devrait perdre des PV");
    EXPECT(barb.hp < 10, "barbare devrait subir le contre-coup");

    BarbarianCamp camp = {.x = 1, .y = 1, .destroyed = 0, .id = 2};
    int destroyed_count = 0;
    Player p = {.gold = 0, .science = 0};

    barbares_destroy_camp(&camp, &destroyed_count, &p);
    EXPECT(camp.destroyed == 1, "camp devrait être détruit");
    EXPECT(destroyed_count == 1, "compteur détruits incorrect");
    EXPECT(p.gold == 5, "bonus or incorrect pour 1er camp");

    free_test_map(&map);
}

static void test_integration_with_map_engine(void)
{
    // Test d'intégration:
    // - on crée une carte avec le moteur map d'Ababacar
    // - on place les unités avec set_unit_at
    // - on vérifie que barbares_move fonctionne bien sur cette carte

    Player p = {.gold = 0, .science = 0, .current_research = TECH_NONE, .unlocked_techs = {0}};
    Map *map = create_map(20, 12, 1234);
    EXPECT(map != NULL, "create_map retourne NULL");
    if (!map) {
        return;
    }

    Unit *barb = (Unit *)malloc(sizeof(Unit));
    Unit *target = (Unit *)malloc(sizeof(Unit));
    EXPECT(barb != NULL && target != NULL, "allocation unités integration");
    if (!barb || !target) {
        free(barb);
        free(target);
        free_map(map);
        return;
    }

    barb->type = UNIT_BARBARIAN;
    barb->hp = 15;
    barb->hp_max = 15;
    barb->attack = 3;
    barb->defense = 2;
    barb->pm = 3;
    barb->pm_max = 3;
    barb->owner = NULL;

    target->type = UNIT_WARRIOR;
    target->hp = 15;
    target->hp_max = 15;
    target->attack = 3;
    target->defense = 2;
    target->pm = 3;
    target->pm_max = 3;
    target->owner = &p;

    map->tiles[1][1].type = TERRAIN_PLAINE;
    map->tiles[1][2].type = TERRAIN_PLAINE;
    map->tiles[1][3].type = TERRAIN_PLAINE;

    set_unit_at(map, 1, 1, barb);
    set_unit_at(map, 3, 1, target);

    Unit *players[1] = {target};
    City *cities[1] = {NULL};

    int old_dist = abs(target->x - barb->x);
    if (abs(target->y - barb->y) > old_dist) {
        old_dist = abs(target->y - barb->y);
    }

    barbares_move(barb, map, cities, 0, players, 1);

    int new_dist = abs(target->x - barb->x);
    if (abs(target->y - barb->y) > new_dist) {
        new_dist = abs(target->y - barb->y);
    }

    EXPECT(new_dist < old_dist, "intégration: barbare doit se rapprocher de la cible");
    EXPECT(map->tiles[barb->y][barb->x].unit == barb, "intégration: map moteur mise à jour après move");

    map->tiles[target->y][target->x].unit = NULL;
    free(target);

    free_map(map);
}

int main(void)
{
    /* appeler les autres suites de tests intégrées */
    extern int run_test_physique(void);
    extern int run_test_config(void);
    run_test_physique();
    run_test_config();
    // Exécution séquentielle des tests unitaires du module.
    test_init_camps();
    test_spawn_unit();
    test_move_barbarian();
    test_move_barbarian_pathfinding_around_wall();
    test_attack_and_destroy();
    test_integration_with_map_engine();
    extern int run_test_difficulty_effects(void);
    if (run_test_difficulty_effects() != 0) {
        g_failed++;
    }

    if (g_failed == 0) {
        puts("[OK] Tous les tests barbares passent.");
        return 0;
    }

    fprintf(stderr, "[FAIL] %d test(s) ont échoué.\n", g_failed);
    return 1;
}
