#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "core/types.h"
#include "core/special_ressources.h"


static int g_failed = 0;

#define EXPECT(cond, msg)                                                   \
    do {                                                                    \
        if (!(cond)) {                                                      \
            fprintf(stderr, "[FAIL][sres] %s\n", (msg));                    \
            g_failed++;                                                     \
        }                                                                   \
    } while (0)

/* ---------- Tests ---------- */

static void test_catalogue_consistency(void)
{
    for (int i = 0; i < NUM_SPECIAL_RESOURCES; i++) {
        EXPECT(SPRES_CATALOG[i].id == (SpecialResourceType)i,
               "L'index doit correspondre au SpecialResourceType");
        EXPECT(SPRES_CATALOG[i].name != NULL,
               "Toute ressource doit avoir un nom");
        EXPECT(SPRES_CATALOG[i].num_valid_terrains >= 1 &&
               SPRES_CATALOG[i].num_valid_terrains <= SPRES_MAX_VALID_TERRAINS,
               "Le nombre de terrains valides doit etre dans la borne");
    }
}

static void test_terrain_compatibility(void)
{
    /* Cas autorisés (conformement au README §4.6) */
    EXPECT(sres_terrain_compatible(SPRES_WHEAT, TERRAIN_PLAINE) == 1,
           "Ble sur Plaine : compatible");
    EXPECT(sres_terrain_compatible(SPRES_IRON, TERRAIN_MONTAGNE) == 1,
           "Fer sur Montagne : compatible");
    EXPECT(sres_terrain_compatible(SPRES_SPICES, TERRAIN_FORET) == 1,
           "Epices sur Foret : compatible");
    EXPECT(sres_terrain_compatible(SPRES_SPICES, TERRAIN_DESERT) == 1,
           "Epices sur Desert : compatible (2eme terrain)");
    EXPECT(sres_terrain_compatible(SPRES_HORSES, TERRAIN_PLAINE) == 1,
           "Chevaux sur Plaine : compatible");
    EXPECT(sres_terrain_compatible(SPRES_HORSES, TERRAIN_TOUNDRA) == 1,
           "Chevaux sur Toundra : compatible (2eme terrain)");
    EXPECT(sres_terrain_compatible(SPRES_SPECIMENS, TERRAIN_TOUNDRA) == 1,
           "Specimens sur Toundra : compatible");

    /* Cas refusés */
    EXPECT(sres_terrain_compatible(SPRES_WHEAT, TERRAIN_EAU) == 0,
           "Ble sur Eau : refuse");
    EXPECT(sres_terrain_compatible(SPRES_WHEAT, TERRAIN_MONTAGNE) == 0,
           "Ble sur Montagne : refuse");
    EXPECT(sres_terrain_compatible(SPRES_IRON, TERRAIN_PLAINE) == 0,
           "Fer sur Plaine : refuse");
    EXPECT(sres_terrain_compatible(SPRES_SPECIMENS, TERRAIN_PLAINE) == 0,
           "Specimens sur Plaine : refuse");
}

static void test_visibility_requires_tech(void)
{
    Player p;
    memset(&p, 0, sizeof(p));

    /* Sans aucune techno : aucune ressource n'est visible. */
    EXPECT(sres_visible(&p, SPRES_WHEAT) == 0,    "Ble invisible sans Agriculture");
    EXPECT(sres_visible(&p, SPRES_IRON) == 0,     "Fer invisible sans Artisanat");
    EXPECT(sres_visible(&p, SPRES_SPICES) == 0,   "Epices invisibles sans Commerce");
    EXPECT(sres_visible(&p, SPRES_SPECIMENS) == 0,"Specimens invisibles sans Ecriture");
    EXPECT(sres_visible(&p, SPRES_HORSES) == 0,   "Chevaux invisibles sans Equitation");

    /* On débloque Agriculture : seul le Blé devient visible. */
    p.unlocked_techs[TECH_AGRICULTURE] = 1;
    EXPECT(sres_visible(&p, SPRES_WHEAT) == 1,
           "Ble visible avec Agriculture");
    EXPECT(sres_visible(&p, SPRES_IRON) == 0,
           "Fer toujours invisible : techno differente");

    /* player == NULL : tout invisible (mode "safe"). */
    EXPECT(sres_visible(NULL, SPRES_WHEAT) == 0,
           "NULL player : tout invisible");
}

static void test_yield_zero_without_tech(void)
{
    Player p;
    memset(&p, 0, sizeof(p));

    TileYield y = sres_yield_for_player(SPRES_WHEAT, &p);
    EXPECT(y.food == 0 && y.production == 0 && y.gold == 0 && y.science == 0,
           "Yield = 0 sans la techno requise");
}

static void test_yield_correct_with_tech(void)
{
    Player p;
    memset(&p, 0, sizeof(p));

    /* Avec Agriculture, le Blé apporte +3 food (README §4.6). */
    p.unlocked_techs[TECH_AGRICULTURE] = 1;
    TileYield y = sres_yield_for_player(SPRES_WHEAT, &p);
    EXPECT(y.food == 3,       "Ble : +3 food avec Agriculture");
    EXPECT(y.production == 0, "Ble : pas de prod");
    EXPECT(y.gold == 0,       "Ble : pas d'or");

    /* Artisanat -> Fer +3 prod */
    p.unlocked_techs[TECH_ARTISANAT] = 1;
    y = sres_yield_for_player(SPRES_IRON, &p);
    EXPECT(y.production == 3, "Fer : +3 production avec Artisanat");

    /* Commerce -> Épices +4 or */
    p.unlocked_techs[TECH_COMMERCE] = 1;
    y = sres_yield_for_player(SPRES_SPICES, &p);
    EXPECT(y.gold == 4, "Epices : +4 or avec Commerce");

    /* Écriture -> Spécimens +3 science */
    p.unlocked_techs[TECH_ECRITURE] = 1;
    y = sres_yield_for_player(SPRES_SPECIMENS, &p);
    EXPECT(y.science == 3, "Specimens : +3 science avec Ecriture");

    /* Équitation -> Chevaux : yield F/P/G/S reste 0, le bonus est en PM
     * (géré ailleurs dans process_city_production). */
    p.unlocked_techs[TECH_EQUITATION] = 1;
    y = sres_yield_for_player(SPRES_HORSES, &p);
    EXPECT(y.food == 0 && y.production == 0 && y.gold == 0 && y.science == 0,
           "Chevaux : yield F/P/G/S = 0 (le +PM est applique ailleurs)");
    EXPECT(SPRES_CATALOG[SPRES_HORSES].bonus_pm == 1,
           "Chevaux : bonus_pm = 1 dans le catalogue");
}

static void test_random_returns_compatible(void)
{
    /* On tire un grand nombre de fois et on verifie que ce qui sort est
     * toujours compatible avec le terrain demande. */
    srand(12345);
    for (int i = 0; i < 200; i++) {
        SpecialResourceType sr = sres_random_for_terrain(TERRAIN_PLAINE);
        EXPECT(sr == SPRES_WHEAT || sr == SPRES_HORSES,
               "Random sur Plaine : doit etre Ble ou Chevaux");
    }
    for (int i = 0; i < 200; i++) {
        SpecialResourceType sr = sres_random_for_terrain(TERRAIN_MONTAGNE);
        EXPECT(sr == SPRES_IRON,
               "Random sur Montagne : doit etre Fer");
    }
    /* Aucune ressource sur l'Eau. */
    EXPECT(sres_random_for_terrain(TERRAIN_EAU) == SPRES_NONE,
           "Random sur Eau : aucune ressource");
}

static void test_invalid_inputs(void)
{
    /* Robustesse : valeurs hors bornes. */
    EXPECT(sres_terrain_compatible(SPRES_NONE, TERRAIN_PLAINE) == 0,
           "SPRES_NONE n'est compatible avec rien");
    EXPECT(sres_get(SPRES_NONE) == NULL,
           "sres_get(SPRES_NONE) renvoie NULL");

    Player p;
    memset(&p, 0, sizeof(p));
    TileYield y = sres_yield_for_player(SPRES_NONE, &p);
    EXPECT(y.food == 0 && y.production == 0 && y.gold == 0 && y.science == 0,
           "Yield pour SPRES_NONE : 0");
}

int run_test_special_resources(void)
{
    test_catalogue_consistency();
    test_terrain_compatibility();
    test_visibility_requires_tech();
    test_yield_zero_without_tech();
    test_yield_correct_with_tech();
    test_random_returns_compatible();
    test_invalid_inputs();

    if (g_failed == 0) {
        puts("[OK] Special resources (EXT-2) tests passed.");
        return 0;
    }
    fprintf(stderr, "[FAIL] %d special_resources test(s) failed.\n", g_failed);
    return 1;
}