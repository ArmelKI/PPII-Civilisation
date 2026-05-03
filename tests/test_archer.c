#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "core/map.h"
#include "core/ressources.h"
#include "core/types.h"

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
            rows[y][x].visibility = FOG_VISIBLE;
            rows[y][x].city = NULL;
            rows[y][x].unit = NULL;
            rows[y][x].camp = NULL;
        }
    }
}

static void free_test_map(Map *map)
{
    if (!map || !map->tiles) {
        return;
    }

    free(map->tiles[0]);
    free(map->tiles);
    map->tiles = NULL;
}

static void test_archer_production_success(void)
{
    Map map;
    init_map(&map, 5, 5);

    Player player = {.gold = 0, .science = 0, .current_research = TECH_NONE, .unlocked_techs = {0}};
    player.unlocked_techs[TECH_CHASSE] = 1;

    City city = {0};
    city.x = 2;
    city.y = 2;
    city.owner = &player;
    city.has_barrack = 1;
    city.current_project = PROJECT_ARCHER;
    city.production_stored = 55;

    map.tiles[2][2].city = &city;

    Unit *player_units[8] = {0};
    int player_unit_count = 0;

    process_city_production(&city, &map, player_units, &player_unit_count);

    EXPECT(player_unit_count == 1, "un Archer doit être produit si les prérequis sont validés");
    EXPECT(player_units[0] != NULL, "le slot unité doit être rempli");
    if (player_units[0]) {
        EXPECT(player_units[0]->type == UNIT_ARCHER, "l'unité créée doit être un Archer");
        EXPECT(player_units[0]->attack == 4, "attaque Archer incorrecte");
        EXPECT(player_units[0]->defense == 1, "defense Archer incorrecte");
        EXPECT(player_units[0]->pm_max == 2, "PM Archer incorrects");
        free(player_units[0]);
    }

    free_test_map(&map);
}

static void test_archer_production_requires_chasse(void)
{
    Map map;
    init_map(&map, 5, 5);

    Player player = {.gold = 0, .science = 0, .current_research = TECH_NONE, .unlocked_techs = {0}};

    City city = {0};
    city.x = 2;
    city.y = 2;
    city.owner = &player;
    city.has_barrack = 1;
    city.current_project = PROJECT_ARCHER;
    city.production_stored = 55;

    map.tiles[2][2].city = &city;

    Unit *player_units[8] = {0};
    int player_unit_count = 0;

    process_city_production(&city, &map, player_units, &player_unit_count);

    EXPECT(player_unit_count == 0, "sans Chasse, l'Archer ne doit pas être produit");
    EXPECT(city.current_project == PROJECT_ARCHER, "le projet Archer doit rester en attente sans prérequis tech");

    free_test_map(&map);
}

int run_test_archer(void)
{
    test_archer_production_success();
    test_archer_production_requires_chasse();

    if (g_failed == 0) {
        puts("[OK] Archer tests passed.");
        return 0;
    }

    fprintf(stderr, "[FAIL] %d test(s) Archer ont echoue.\n", g_failed);
    return 1;
}
