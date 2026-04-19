#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "core/barbares.h"
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
        fprintf(stderr, "Erreur allocation map de test\n");
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

static void free_map(Map *map)
{
    if (!map || !map->tiles) {
        return;
    }

    free(map->tiles[0]);
    free(map->tiles);
    map->tiles = NULL;
}

static void test_init_camps(void)
{
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
            EXPECT(camps[i].destroyed == 0, "campement pas actif apres placement");
            EXPECT(camps[i].x >= 0 && camps[i].x < map.width, "x campement invalide");
            EXPECT(camps[i].y >= 0 && camps[i].y < map.height, "y campement invalide");
            EXPECT(map.tiles[camps[i].y][camps[i].x].type != TERRAIN_EAU, "campement place sur eau");
            EXPECT(!(abs(camps[i].x - city.x) < 5 && abs(camps[i].y - city.y) < 5),
                   "campement trop proche de la ville de depart");
        }
        barbares_cleanup(camps, 3);
    }

    free_map(&map);
}

static void test_spawn_unit(void)
{
    Map map;
    init_map(&map, 8, 8);

    Unit *barb_slots[4] = {0};
    int num = 0;

    BarbarianCamp camp = {.x = 3, .y = 4, .destroyed = 0, .id = 7};

    int ok = barbares_spawn_unit(&camp, &map, barb_slots, &num, 4);
    EXPECT(ok == 1, "spawn devrait reussir");
    EXPECT(num == 1, "num_units devrait etre incremente");
    EXPECT(barb_slots[0] != NULL, "slot barbare non rempli");
    EXPECT(map.tiles[4][3].unit == barb_slots[0], "tuile camp non occupee par le barbare");

    ok = barbares_spawn_unit(&camp, &map, barb_slots, &num, 4);
    EXPECT(ok == 0, "spawn doit echouer si tuile camp occupee");

    map.tiles[4][3].unit = NULL;
    camp.destroyed = 1;
    ok = barbares_spawn_unit(&camp, &map, barb_slots, &num, 4);
    EXPECT(ok == 0, "spawn doit echouer si camp detruit");

    camp.destroyed = 0;
    num = 4;
    ok = barbares_spawn_unit(&camp, &map, barb_slots, &num, 4);
    EXPECT(ok == 0, "spawn doit echouer si plafond atteint");

    for (int i = 0; i < 4; ++i) {
        free(barb_slots[i]);
        barb_slots[i] = NULL;
    }

    free_map(&map);
}

static void test_move_barbarian(void)
{
    Map map;
    init_map(&map, 10, 10);

    Unit barb = {.type = UNIT_BARBARIAN, .hp = 10, .pm = 3, .pm_max = 3, .x = 5, .y = 5};
    map.tiles[5][5].unit = &barb;

    Unit target = {.type = UNIT_WARRIOR, .hp = 10, .x = 7, .y = 5};
    Unit *targets[1] = {&target};

    City *cities[1] = {NULL};

    barbares_move(&barb, &map, cities, 0, targets, 1);
    EXPECT(barb.x == 6 && barb.y == 5, "barbare devrait avancer d'une case vers la cible");
    EXPECT(barb.pm == 2, "cout PM incorrect sur plaine");
    EXPECT(map.tiles[5][6].unit == &barb, "nouvelle tuile non mise a jour");
    EXPECT(map.tiles[5][5].unit == NULL, "ancienne tuile non videe");

    map.tiles[5][6].unit = NULL;
    barb.x = 5;
    barb.y = 5;
    barb.pm = 3;
    map.tiles[5][5].unit = &barb;

    target.x = 5;
    target.y = 7;
    map.tiles[6][5].type = TERRAIN_EAU;

    barbares_move(&barb, &map, cities, 0, targets, 1);
    EXPECT(barb.x == 5 && barb.y == 5, "barbare ne doit pas entrer sur eau");

    free_map(&map);
}

static void test_attack_and_destroy(void)
{
    Map map;
    init_map(&map, 8, 8);

    Unit barb = {.type = UNIT_BARBARIAN, .hp = 10, .attack = 3, .defense = 1, .x = 3, .y = 3};
    map.tiles[3][3].unit = &barb;

    Unit enemy = {.type = UNIT_WARRIOR, .hp = 10, .attack = 2, .defense = 1, .x = 4, .y = 3};
    map.tiles[3][4].unit = &enemy;
    Unit *player_units[1] = {&enemy};
    City *cities[1] = {NULL};

    int fought = barbares_attack_if_adjacent(&barb, &map, cities, 0, player_units, 1);
    EXPECT(fought == 1, "combat adjacent devrait etre detecte");
    EXPECT(enemy.hp < 10, "ennemi devrait perdre des PV");
    EXPECT(barb.hp < 10, "barbare devrait subir le contre-coup");

    BarbarianCamp camp = {.x = 1, .y = 1, .destroyed = 0, .id = 2};
    int destroyed_count = 0;
    Player p = {.gold = 0, .science = 0};

    barbares_destroy_camp(&camp, &destroyed_count, &p);
    EXPECT(camp.destroyed == 1, "camp devrait etre detruit");
    EXPECT(destroyed_count == 1, "compteur detruits incorrect");
    EXPECT(p.gold == 5, "bonus or incorrect pour 1er camp");

    free_map(&map);
}

int main(void)
{
    test_init_camps();
    test_spawn_unit();
    test_move_barbarian();
    test_attack_and_destroy();

    if (g_failed == 0) {
        puts("[OK] Tous les tests barbares passent.");
        return 0;
    }

    fprintf(stderr, "[FAIL] %d test(s) ont echoue.\n", g_failed);
    return 1;
}
