#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "core/config.h"
#include "core/map.h"
#include "core/ressources.h"

static int g_failed = 0;

#define EXPECT(cond, msg)                                                       \
    do {                                                                        \
        if (!(cond)) {                                                          \
            fprintf(stderr, "[FAIL] %s\n", (msg));                          \
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

static void test_production_multiplier(void)
{
    GameConfig cfg;
    game_config_init_default(&cfg);
    cfg.production_multiplier = 2.0f;
    game_config_set_active(&cfg);

    Map map;
    init_map(&map, 3, 3);

    Player player = {.gold = 0, .science = 0, .current_research = TECH_NONE, .unlocked_techs = {0}};
    City city = {0};
    city.population = 1;
    city.x = 1;
    city.y = 1;
    city.owner = &player;
    map.tiles[1][1].city = &city;

    process_city_resources(&city, &map);

    EXPECT(city.production_stored == 18, "la production doit être doublée par le multiplicateur");

    free_test_map(&map);
}

static void test_growth_and_famine(void)
{
    GameConfig cfg;
    game_config_init_default(&cfg);
    cfg.growth_threshold_multiplier = 0.5f;
    cfg.famine_enabled = false;
    game_config_set_active(&cfg);

    Map map;
    init_map(&map, 3, 3);

    City city = {0};
    city.population = 1;
    city.food_stored = 15;
    city.x = 1;
    city.y = 1;
    map.tiles[1][1].city = &city;

    process_city_growth(&city, &map);
    EXPECT(city.population == 2, "le seuil de croissance doit être réduit par le multiplicateur");
    EXPECT(city.food_stored == 0, "la nourriture doit être remise à zéro après croissance");

    city.population = 2;
    city.food_stored = -3;
    process_city_growth(&city, &map);
    EXPECT(city.population == 2, "la famine doit être désactivée sur ce preset");
    EXPECT(city.food_stored == 0, "le déficit doit être neutralisé si la famine est désactivée");

    free_test_map(&map);
}

static void test_barbarian_profile(void)
{
    GameConfig cfg;
    game_config_init_default(&cfg);
    cfg.barbarian_spawn_multiplier = 2.0f;
    game_config_set_active(&cfg);

    EXPECT(game_config_barbarian_spawn_chance() == 70, "la chance d'apparition barbare doit suivre le multiplicateur");

    Unit barbarian = {0};
    game_config_apply_barbarian_stats(&barbarian);
    EXPECT(barbarian.hp == 30, "les PV barbares doivent être renforcés");
    EXPECT(barbarian.attack == 6, "l'attaque barbare doit être renforcée");
    EXPECT(barbarian.pm == 6, "les PM barbares doivent être renforcés");

    game_config_init_default(&cfg);
    game_config_set_active(&cfg);
}

int run_test_difficulty_effects(void)
{
    test_production_multiplier();
    test_growth_and_famine();
    test_barbarian_profile();

    if (g_failed == 0) {
        puts("[OK] Difficulty gameplay tests passed.");
        return 0;
    }

    fprintf(stderr, "[FAIL] %d test(s) failed.\n", g_failed);
    return 1;
}
