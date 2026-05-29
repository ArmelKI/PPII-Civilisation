#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "core/types.h"
#include "core/game.h"
#include "core/wonder.h"

static int g_failed = 0;

#define EXPECT(cond, msg)                                                   \
    do {                                                                    \
        if (!(cond)) {                                                      \
            fprintf(stderr, "[FAIL][wonders] %s\n", (msg));                 \
            g_failed++;                                                     \
        }                                                                   \
    } while (0)

/* ---------- Helpers de test ---------- */

/* Construit un GameState minimal avec une map W*H, tout en TERRAIN_PLAINE,
 * sans ville ni unité. Les nouveaux champs Tile sont bien initialisés à -1. */
static void make_minimal_gamestate(GameState *g, int w, int h)
{
    memset(g, 0, sizeof(*g));

    Tile *buffer = (Tile *)calloc((size_t)w * (size_t)h, sizeof(Tile));
    Tile **rows = (Tile **)malloc((size_t)h * sizeof(Tile *));
    for (int y = 0; y < h; y++) {
        rows[y] = &buffer[y * w];
        for (int x = 0; x < w; x++) {
            rows[y][x].type = TERRAIN_PLAINE;
            rows[y][x].visibility = FOG_VISIBLE;
            rows[y][x].city = NULL;
            rows[y][x].unit = NULL;
            rows[y][x].camp = NULL;
            rows[y][x].wonder = WONDER_NONE;
            rows[y][x].special_ressources = SPRES_NONE;
        }
    }
    g->map.width = w;
    g->map.height = h;
    g->map.tiles = rows;

    for (int i = 0; i < NUM_WONDERS; i++) {
        g->wonder_host_city_id[i] = -1;
    }
}

static void free_minimal_gamestate(GameState *g)
{
    if (g->map.tiles) {
        free(g->map.tiles[0]); /* buffer continu */
        free(g->map.tiles);
        g->map.tiles = NULL;
    }
}

/* ---------- Tests ---------- */

static void test_catalogue_consistency(void)
{
    for (int i = 0; i < NUM_WONDERS; i++) {
        EXPECT(WONDER_CATALOG[i].id == (WonderType)i,
               "L'index du catalogue doit correspondre au WonderType");
        EXPECT(WONDER_CATALOG[i].cost > 0,
               "Toute merveille doit avoir un cout > 0");
        EXPECT(WONDER_CATALOG[i].name != NULL,
               "Toute merveille doit avoir un nom");
    }
    /* Conformité au README §4.5 : couts officiels des merveilles. */
    EXPECT(wonder_cost(WONDER_PYRAMIDS)       == 200, "Pyramides coutent 200");
    EXPECT(wonder_cost(WONDER_GREAT_LIBRARY)  == 250, "Grande Biblio coute 250");
    EXPECT(wonder_cost(WONDER_COLISEUM)       == 180, "Colisee coute 180");
    EXPECT(wonder_cost(WONDER_BABEL)          == 300, "Tour de Babel coute 300");
    EXPECT(wonder_cost(WONDER_HANGING_GARDEN) == 220, "Jardins coutent 220");
}

static void test_terrain_compatibility(void)
{
    EXPECT(wonder_terrain_compatible(WONDER_PYRAMIDS, TERRAIN_DESERT) == 1,
           "Pyramides sur Desert : compatible");
    EXPECT(wonder_terrain_compatible(WONDER_PYRAMIDS, TERRAIN_PLAINE) == 0,
           "Pyramides sur Plaine : refusee");
    EXPECT(wonder_terrain_compatible(WONDER_COLISEUM, TERRAIN_PLAINE) == 1,
           "Colisee sur Plaine : compatible");
    EXPECT(wonder_terrain_compatible(WONDER_BABEL, TERRAIN_MONTAGNE) == 1,
           "Babel sur Montagne : compatible");
    EXPECT(wonder_terrain_compatible(WONDER_HANGING_GARDEN, TERRAIN_FORET) == 1,
           "Jardins sur Foret : compatible");
    EXPECT(wonder_terrain_compatible(WONDER_GREAT_LIBRARY, TERRAIN_TOUNDRA) == 1,
           "Grande Biblio sur Toundra : compatible");
}

static void test_project_wonder_conversions(void)
{
    EXPECT(wonder_to_project(WONDER_PYRAMIDS) == PROJECT_WONDER_PYRAMIDS,
           "Conversion WonderType -> ProjectType");
    EXPECT(project_to_wonder(PROJECT_WONDER_COLISEUM) == WONDER_COLISEUM,
           "Conversion ProjectType -> WonderType");
    EXPECT(project_to_wonder(PROJECT_WARRIOR) == WONDER_NONE,
           "Un projet non-merveille renvoie WONDER_NONE");
}

static void test_can_build_finds_compatible_tile(void)
{
    GameState g;
    make_minimal_gamestate(&g, 10, 10);

    /* On place une ville fictive au centre avec population 5 (rayon 1). */
    City city = { .x = 5, .y = 5, .population = 5, .id = 42 };

    /* Sans terrain compatible (tout en plaine), Pyramides impossibles. */
    int tx = -1, ty = -1;
    EXPECT(wonder_can_build_in_city(&g, &city, WONDER_PYRAMIDS, &tx, &ty) == 0,
           "Pyramides : aucun Desert dans le rayon -> refus");

    /* On met UNE seule case desert dans le rayon. */
    g.map.tiles[6][6].type = TERRAIN_DESERT;
    EXPECT(wonder_can_build_in_city(&g, &city, WONDER_PYRAMIDS, &tx, &ty) == 1,
           "Pyramides : Desert dans le rayon -> autorise");
    EXPECT(tx == 6 && ty == 6, "La case retournee doit etre [6,6]");

    free_minimal_gamestate(&g);
}

static void test_can_build_skips_center(void)
{
    GameState g;
    make_minimal_gamestate(&g, 10, 10);

    /* La case CENTRALE de la ville en desert ne doit PAS etre utilisee
     * (la merveille n'occupe pas le centre, qui porte deja le batiment). */
    City city = { .x = 5, .y = 5, .population = 5, .id = 1 };
    g.map.tiles[5][5].type = TERRAIN_DESERT;

    int tx = -1, ty = -1;
    EXPECT(wonder_can_build_in_city(&g, &city, WONDER_PYRAMIDS, &tx, &ty) == 0,
           "Pyramides : ne se posent pas sur la case centrale de la ville");

    free_minimal_gamestate(&g);
}

static void test_mark_and_unicity(void)
{
    GameState g;
    make_minimal_gamestate(&g, 5, 5);

    EXPECT(wonder_is_built(&g, WONDER_PYRAMIDS) == 0,
           "Aucune merveille construite au depart");

    wonder_mark_built(&g, WONDER_PYRAMIDS, 2, 2, 42);

    EXPECT(wonder_is_built(&g, WONDER_PYRAMIDS) == 1,
           "Apres mark_built, la merveille est construite");
    EXPECT(g.wonder_x[WONDER_PYRAMIDS] == 2 && g.wonder_y[WONDER_PYRAMIDS] == 2,
           "Les coordonnees stockees sont [2,2]");
    EXPECT(g.wonder_host_city_id[WONDER_PYRAMIDS] == 42,
           "L'id de la ville hote est conserve");
    EXPECT(g.map.tiles[2][2].wonder == WONDER_PYRAMIDS,
           "La tuile est marquee avec la merveille");

    /* Une seule merveille de chaque type, les autres restent libres. */
    EXPECT(wonder_is_built(&g, WONDER_COLISEUM) == 0,
           "Les autres merveilles restent non-construites");

    free_minimal_gamestate(&g);
}

static void test_destroy_releases_tile(void)
{
    GameState g;
    make_minimal_gamestate(&g, 5, 5);

    wonder_mark_built(&g, WONDER_COLISEUM, 3, 3, 7);
    EXPECT(g.map.tiles[3][3].wonder == WONDER_COLISEUM,
           "Apres mark, la tuile a la merveille");

    wonder_destroy(&g, WONDER_COLISEUM);
    EXPECT(wonder_is_built(&g, WONDER_COLISEUM) == 0,
           "Apres destroy, la merveille n'est plus construite");
    EXPECT(g.map.tiles[3][3].wonder == WONDER_NONE,
           "Apres destroy, la tuile est liberee");

    free_minimal_gamestate(&g);
}

static void test_handle_city_destroyed(void)
{
    GameState g;
    make_minimal_gamestate(&g, 5, 5);

    /* La ville 42 construit Pyramides ET Colisée, la ville 7 construit Babel. */
    wonder_mark_built(&g, WONDER_PYRAMIDS, 1, 1, 42);
    wonder_mark_built(&g, WONDER_COLISEUM, 2, 2, 42);
    wonder_mark_built(&g, WONDER_BABEL,    3, 3, 7);

    /* La ville 42 est rasee. Seules ses 2 merveilles doivent disparaitre. */
    wonder_handle_city_destroyed(&g, 42);

    EXPECT(wonder_is_built(&g, WONDER_PYRAMIDS) == 0,
           "Pyramides (ville 42) detruite avec la ville");
    EXPECT(wonder_is_built(&g, WONDER_COLISEUM) == 0,
           "Colisee (ville 42) detruite avec la ville");
    EXPECT(wonder_is_built(&g, WONDER_BABEL) == 1,
           "Babel (ville 7) survit");

    free_minimal_gamestate(&g);
}

static void test_modifiers(void)
{
    GameState g;
    make_minimal_gamestate(&g, 5, 5);

    /* Etat initial : tous les multiplicateurs sont neutres. */
    EXPECT(wonder_food_multiplier(&g)             == 1.0f, "Food mult = 1.0 au depart");
    EXPECT(wonder_growth_threshold_multiplier(&g) == 1.0f, "Growth mult = 1.0 au depart");
    EXPECT(wonder_tech_cost_multiplier(&g)        == 1.0f, "Tech mult = 1.0 au depart");
    EXPECT(wonder_extra_gold_per_city(&g)         == 0,    "Gold/ville = 0 au depart");

    /* Pyramides -> food x 1.5 (README §4.5) */
    wonder_mark_built(&g, WONDER_PYRAMIDS, 0, 0, 0);
    EXPECT(wonder_food_multiplier(&g) == 1.5f, "Food x1.5 apres Pyramides");

    /* Jardins -> seuil x 0.8 (README §4.5) */
    wonder_mark_built(&g, WONDER_HANGING_GARDEN, 0, 1, 0);
    EXPECT(wonder_growth_threshold_multiplier(&g) == 0.8f, "Seuil x0.8 apres Jardins");

    /* Babel -> cout tech x 0.75 (README §4.5) */
    wonder_mark_built(&g, WONDER_BABEL, 0, 2, 0);
    EXPECT(wonder_tech_cost_multiplier(&g) == 0.75f, "Tech x0.75 apres Babel");

    /* Colisee -> +2 or par ville (README §4.5) */
    wonder_mark_built(&g, WONDER_COLISEUM, 0, 3, 0);
    EXPECT(wonder_extra_gold_per_city(&g) == 2, "+2 or/ville apres Colisee");

    free_minimal_gamestate(&g);
}

static void test_null_gamestate_safe(void)
{
    /* Tous les helpers doivent etre safe avec game == NULL et renvoyer
     * des valeurs neutres. Ca permet aux tests existants de tourner sans
     * fournir de GameState. */
    EXPECT(wonder_food_multiplier(NULL)             == 1.0f, "NULL game : food neutre");
    EXPECT(wonder_growth_threshold_multiplier(NULL) == 1.0f, "NULL game : growth neutre");
    EXPECT(wonder_tech_cost_multiplier(NULL)        == 1.0f, "NULL game : tech neutre");
    EXPECT(wonder_extra_gold_per_city(NULL)         == 0,    "NULL game : or neutre");
    EXPECT(wonder_is_built(NULL, WONDER_PYRAMIDS)   == 0,    "NULL game : aucune construite");
}

int run_test_wonders(void)
{
    test_catalogue_consistency();
    test_terrain_compatibility();
    test_project_wonder_conversions();
    test_can_build_finds_compatible_tile();
    test_can_build_skips_center();
    test_mark_and_unicity();
    test_destroy_releases_tile();
    test_handle_city_destroyed();
    test_modifiers();
    test_null_gamestate_safe();

    if (g_failed == 0) {
        puts("[OK] Wonders (EXT-2) tests passed.");
        return 0;
    }
    fprintf(stderr, "[FAIL] %d wonders test(s) failed.\n", g_failed);
    return 1;
}