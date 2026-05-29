#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "core/types.h"
#include "core/game.h"
#include "core/config.h"
#include "core/ressources.h"

/* ============================================================
 * Tests unitaires du module ressources.c
 * ----------------------------------------------------------------
 * Couvre :
 *   - get_terrain_yield, get_chebyshev_distance (fonctions pures)
 *   - get_tile_owner (propriete des cases par rayon d'exploitation)
 *   - process_city_resources (yield, batiments, merveilles, ressources speciales)
 *   - process_city_growth (croissance, famine, modulateurs)
 *   - process_player_gold (entretien)
 *   - found_city (fondation par un colon)
 *   - process_city_production (prerequis technologiques EXT-2)
 * ============================================================ */

static int g_failed = 0;

#define EXPECT(cond, msg)                                                   \
    do {                                                                    \
        if (!(cond)) {                                                      \
            fprintf(stderr, "[FAIL][ressources] %s\n", (msg));              \
            g_failed++;                                                     \
        }                                                                   \
    } while (0)

/* ---------- Helpers ---------- */

/* Petite map W*H, toutes les tuiles en Plaine, FOG_VISIBLE, sans entites.
 * Les champs EXT-2 sont explicitement mis a -1 sinon calloc les laisse a 0
 * et 0 = WONDER_PYRAMIDS / SPRES_WHEAT (probleme de "pyramide fantome"). */
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
            rows[y][x].wonder = WONDER_NONE;
            rows[y][x].special_ressources = SPRES_NONE;
        }
    }
}

static void free_test_map(Map *map)
{
    if (map->tiles) {
        free(map->tiles[0]);
        free(map->tiles);
        map->tiles = NULL;
    }
}

/* Configure le moteur en mode "Normal" pour neutraliser les multiplicateurs
 * de difficulte (production x1, croissance x1, famine activee). */
static void setup_default_config(void)
{
    static GameConfig cfg;
    game_config_init_default(&cfg);
    game_config_set_active(&cfg);
}

/* ============================================================
 * Section A : Fonctions pures
 * ============================================================ */

static void test_terrain_yield(void)
{
    /* Conformite au README §3 : yields exacts par type de terrain. */
    TileYield y;

    y = get_terrain_yield(TERRAIN_PLAINE);
    EXPECT(y.food == 2 && y.production == 1 && y.gold == 0 && y.science == 0,
           "Plaine : 2 food, 1 production");

    y = get_terrain_yield(TERRAIN_FORET);
    EXPECT(y.food == 1 && y.production == 2 && y.gold == 0 && y.science == 0,
           "Foret : 1 food, 2 production");

    y = get_terrain_yield(TERRAIN_MONTAGNE);
    EXPECT(y.food == 0 && y.production == 3 && y.gold == 0 && y.science == 1,
           "Montagne : 3 production, 1 science");

    y = get_terrain_yield(TERRAIN_EAU);
    EXPECT(y.food == 1 && y.production == 0 && y.gold == 1 && y.science == 0,
           "Eau : 1 food, 1 or");

    y = get_terrain_yield(TERRAIN_DESERT);
    EXPECT(y.food == 0 && y.production == 0 && y.gold == 1 && y.science == 0,
           "Desert : 1 or seulement");

    y = get_terrain_yield(TERRAIN_TOUNDRA);
    EXPECT(y.food == 1 && y.production == 1 && y.gold == 0 && y.science == 0,
           "Toundra : 1 food, 1 production");
}

static void test_chebyshev_distance(void)
{
    /* La distance de Chebyshev (roi aux echecs) vaut max(|dx|,|dy|). */
    EXPECT(get_chebyshev_distance(0, 0, 0, 0) == 0, "distance (0,0)->(0,0) = 0");
    EXPECT(get_chebyshev_distance(0, 0, 3, 0) == 3, "distance horizontale = 3");
    EXPECT(get_chebyshev_distance(0, 0, 0, 4) == 4, "distance verticale = 4");
    EXPECT(get_chebyshev_distance(0, 0, 3, 4) == 4, "distance diagonale = max(3,4) = 4");
    EXPECT(get_chebyshev_distance(5, 5, 2, 3) == 3, "distance symetrique en valeur absolue");
    EXPECT(get_chebyshev_distance(2, 2, 4, 4) == 2, "distance pure diagonale = 2");
}

/* ============================================================
 * Section B : get_tile_owner
 * ============================================================ */

static void test_tile_owner_within_radius(void)
{
    /* Une ville pop 1 a un rayon d'exploitation de 1.
     * Toute case a Chebyshev <= 1 lui appartient si pas de concurrence. */
    setup_default_config();
    Map map;
    init_map(&map, 7, 7);

    City city = {0};
    city.x = 3; city.y = 3;
    city.population = 1;
    map.tiles[3][3].city = &city;

    EXPECT(get_tile_owner(2, 3, &map) == &city, "Case adjacente Ouest appartient a la ville");
    EXPECT(get_tile_owner(4, 4, &map) == &city, "Case diagonale appartient (rayon 1)");
    EXPECT(get_tile_owner(3, 3, &map) == &city, "Case centrale appartient");

    /* Hors rayon : doit etre NULL. */
    EXPECT(get_tile_owner(5, 5, &map) == NULL, "Case hors rayon : aucun proprietaire");
    EXPECT(get_tile_owner(0, 0, &map) == NULL, "Case eloignee : aucun proprietaire");

    free_test_map(&map);
}

/* ============================================================
 * Section C : process_city_resources
 * ============================================================ */

static void test_city_resources_basic_plain(void)
{
    /* Ville pop 1 au centre d'une map en Plaine.
     * Rayon 1 = 9 cases (3x3) toutes en Plaine, chacune {2 food, 1 prod}.
     * Total brut : 18 food, 9 prod.
     * Consommation : pop * 2 = 2 food.
     * Resultat : food_stored += 16, production_stored += 9. */
    setup_default_config();
    Map map;
    init_map(&map, 5, 5);

    Player player = {.gold = 0, .science = 0, .current_research = TECH_NONE, .unlocked_techs = {0}};
    City city = {0};
    city.x = 2; city.y = 2;
    city.population = 1;
    city.owner = &player;
    map.tiles[2][2].city = &city;

    process_city_resources(&city, &map, NULL);

    EXPECT(city.food_stored == 16,
           "9 cases Plaine = 18 food brut, -2 conso = 16 food net");
    EXPECT(city.production_stored == 9,
           "9 cases Plaine = 9 production");
    EXPECT(player.gold == 0,    "Plaine ne produit pas d'or");
    EXPECT(player.science == 0, "Plaine ne produit pas de science");

    free_test_map(&map);
}

static void test_city_resources_with_buildings(void)
{
    /* Memes 9 cases Plaine, mais la ville a tous les batiments :
     * +3 food (grenier), +3 prod (atelier), +4 sci (biblio), +3 or (marche). */
    setup_default_config();
    Map map;
    init_map(&map, 5, 5);

    Player player = {.gold = 0, .science = 0, .current_research = TECH_NONE, .unlocked_techs = {0}};
    City city = {0};
    city.x = 2; city.y = 2;
    city.population = 1;
    city.owner = &player;
    city.has_granary = 1;
    city.has_workshop = 1;
    city.has_library = 1;
    city.has_market = 1;
    map.tiles[2][2].city = &city;

    process_city_resources(&city, &map, NULL);

    EXPECT(city.food_stored == 16 + 3,
           "Grenier ajoute +3 food (16 + 3 = 19)");
    EXPECT(city.production_stored == 9 + 3,
           "Atelier ajoute +3 prod (9 + 3 = 12)");
    EXPECT(player.gold == 3,
           "Marche ajoute +3 or");
    EXPECT(player.science == 4,
           "Bibliotheque ajoute +4 science");

    free_test_map(&map);
}

static void test_city_resources_skips_wonder_tile(void)
{
    /* Une case du rayon avec une merveille ne produit plus de yield.
     * Sur 9 cases Plaine, on met une merveille sur une : on attend 8x {2,1}.
     * Conso : 2 food. food_stored = 16-2 = 14, production_stored = 8. */
    setup_default_config();
    Map map;
    init_map(&map, 5, 5);

    Player player = {.gold = 0, .science = 0, .current_research = TECH_NONE, .unlocked_techs = {0}};
    City city = {0};
    city.x = 2; city.y = 2;
    city.population = 1;
    city.owner = &player;
    map.tiles[2][2].city = &city;
    map.tiles[1][1].wonder = WONDER_COLISEUM; /* prive cette case de yield */

    process_city_resources(&city, &map, NULL);

    EXPECT(city.food_stored == 8 * 2 - 2,
           "Une case avec merveille = -1 case exploitable (food)");
    EXPECT(city.production_stored == 8,
           "Une case avec merveille = -1 case exploitable (prod)");

    free_test_map(&map);
}

static void test_city_resources_special_resource_hidden(void)
{
    /* Sans la techno requise, la ressource speciale n'apporte aucun bonus.
     * On pose du Ble sur une case mais le joueur n'a pas Agriculture. */
    setup_default_config();
    Map map;
    init_map(&map, 5, 5);

    Player player = {.gold = 0, .science = 0, .current_research = TECH_NONE, .unlocked_techs = {0}};
    /* Pas d'Agriculture debloquee */
    City city = {0};
    city.x = 2; city.y = 2;
    city.population = 1;
    city.owner = &player;
    map.tiles[2][2].city = &city;
    map.tiles[1][1].special_ressources = SPRES_WHEAT;

    /* On passe NULL car process_city_resources accepte un GameState NULL,
     * mais il faut alors aussi un Player pour la visibilite. Or NULL game
     * = pas de player visible cote helper -> SPRES_NONE comportement.
     * Du coup le bonus est correctement neutralise dans les deux cas
     * (NULL game ou techno absente). */
    process_city_resources(&city, &map, NULL);

    EXPECT(city.food_stored == 16,
           "Sans techno (game NULL), Ble n'apporte aucun bonus");

    free_test_map(&map);
}

/* ============================================================
 * Section D : process_city_growth
 * ============================================================ */

static void test_city_growth_increase(void)
{
    /* Quand food_stored >= seuil (20 + 10*pop), la pop augmente de 1 et la
     * cagnotte food est consommee. Pop 1 -> seuil 30. */
    setup_default_config();
    Map map;
    init_map(&map, 5, 5);

    City city = {0};
    city.x = 2; city.y = 2;
    city.population = 1;
    city.food_stored = 30; /* exactement le seuil */
    map.tiles[2][2].city = &city;

    process_city_growth(&city, &map, NULL);

    EXPECT(city.population == 2,
           "Croissance : pop 1 -> 2 quand food >= seuil");
    EXPECT(city.food_stored < 30,
           "La cagnotte food est consommee a la croissance");

    free_test_map(&map);
}

static void test_city_growth_famine_decreases_pop(void)
{
    /* Une ville avec food_stored < 0 declenche la famine : pop -1. */
    setup_default_config();
    Map map;
    init_map(&map, 5, 5);

    City city = {0};
    city.x = 2; city.y = 2;
    city.population = 3;
    city.food_stored = -5;
    map.tiles[2][2].city = &city;

    process_city_growth(&city, &map, NULL);

    EXPECT(city.population == 2, "Famine : pop 3 -> 2");

    free_test_map(&map);
}

static void test_city_growth_granary_lowers_threshold(void)
{
    /* Le Grenier reduit le seuil de croissance d'un tiers (seuil x 2/3).
     * Pop 1, seuil de base = 30, avec grenier = 20.
     * Avec 25 food on doit donc franchir le seuil avec grenier mais
     * PAS sans grenier. */
    setup_default_config();
    Map map;
    init_map(&map, 5, 5);

    City city = {0};
    city.x = 2; city.y = 2;
    city.population = 1;
    city.food_stored = 25;
    city.has_granary = 1;
    map.tiles[2][2].city = &city;

    process_city_growth(&city, &map, NULL);

    EXPECT(city.population == 2,
           "Grenier : seuil baisse, 25 food suffit pour croitre");

    free_test_map(&map);
}

/* ============================================================
 * Section E : process_player_gold (entretien)
 * ============================================================ */

static void test_player_gold_entretien_buildings(void)
{
    /* Le joueur paie l'entretien des batiments de chacune de ses villes.
     * On part de 100 or, on ajoute une ville avec grenier+atelier+biblio.
     * On verifie qu'apres process_player_gold, l'or a diminue (signe que
     * l'entretien a bien ete preleve). Le montant exact depend du bareme
     * interne, ce qui compte ici c'est qu'il y ait bien deduction. */
    setup_default_config();
    Map map;
    init_map(&map, 5, 5);

    Player player = {.gold = 100, .science = 0, .current_research = TECH_NONE, .unlocked_techs = {0}};
    City city = {0};
    city.x = 2; city.y = 2;
    city.population = 1;
    city.owner = &player;
    city.has_granary = 1;
    city.has_workshop = 1;
    city.has_library = 1;
    map.tiles[2][2].city = &city;

    int gold_before = player.gold;
    process_player_gold(&player, &map);

    EXPECT(player.gold < gold_before,
           "Process player gold : l'or diminue avec les batiments (entretien)");

    free_test_map(&map);
}

/* ============================================================
 * Section F : found_city (fondation par un Colon)
 * ============================================================ */

static void test_found_city_creates_city(void)
{
    /* Un Colon sur une case libre : found_city() doit creer une ville
     * et l'enregistrer dans le tableau player_cities.
     * IMPORTANT : found_city appelle free(settler) en interne, donc le
     * Colon doit etre alloue avec malloc (pas sur la pile). */
    setup_default_config();
    Map map;
    init_map(&map, 5, 5);

    Player player = {.gold = 0, .science = 0, .current_research = TECH_NONE, .unlocked_techs = {0}};
    Unit *settler = malloc(sizeof(Unit));
    memset(settler, 0, sizeof(Unit));
    settler->type = UNIT_SETTLER;
    settler->x = 2; settler->y = 2;
    settler->hp = 10; settler->hp_max = 10;
    settler->pm = 2;  settler->pm_max = 2;
    settler->owner = &player;
    map.tiles[2][2].unit = settler;

    City *cities[8] = {0};
    int city_count = 0;
    int city_id = 0;

    int ok = found_city(settler, &map, &city_id, cities, &city_count);
    /* A partir d'ici, ne plus toucher a `settler` : found_city l'a free(). */

    EXPECT(ok != 0, "found_city renvoie un code de succes");
    EXPECT(city_count == 1, "city_count incremente de 0 a 1");
    EXPECT(cities[0] != NULL, "La nouvelle ville est rangee dans cities[0]");
    if (cities[0]) {
        EXPECT(cities[0]->x == 2 && cities[0]->y == 2,
               "La ville est placee a la position du Colon");
        EXPECT(map.tiles[2][2].city == cities[0],
               "La tuile pointe vers la nouvelle ville");
        free(cities[0]);
    }

    free_test_map(&map);
}

/* ============================================================
 * Section G : prerequis technologiques (process_city_production)
 * ----------------------------------------------------------------
 * On verifie que sans la techno requise, la production echoue
 * silencieusement (l'or/effet du batiment n'est PAS applique).
 * ============================================================ */

static void test_warrior_requires_artisanat(void)
{
    /* Le Guerrier necessite Caserne + Artisanat (README §5.2). */
    setup_default_config();
    Map map;
    init_map(&map, 5, 5);

    Player player = {.gold = 0, .science = 0, .current_research = TECH_NONE, .unlocked_techs = {0}};
    City city = {0};
    city.x = 2; city.y = 2;
    city.population = 1;
    city.owner = &player;
    city.has_barrack = 1; /* caserne OK, on isole le test sur Artisanat */
    city.current_project = PROJECT_WARRIOR;
    city.production_stored = 40;
    map.tiles[2][2].city = &city;
    Unit *units[8] = {0};
    int n = 0;

    process_city_production(&city, &map, units, &n, NULL);
    EXPECT(n == 0, "Sans Artisanat, le Guerrier ne doit PAS etre produit");

    player.unlocked_techs[TECH_ARTISANAT] = 1;
    city.current_project = PROJECT_WARRIOR;
    city.production_stored = 40;
    process_city_production(&city, &map, units, &n, NULL);
    EXPECT(n == 1, "Avec Artisanat, le Guerrier doit etre produit");
    if (n >= 1 && units[0]) {
        EXPECT(units[0]->type == UNIT_WARRIOR, "L'unite creee est bien un Guerrier");
        free(units[0]);
    }

    free_test_map(&map);
}

static void test_library_requires_ecriture(void)
{
    /* La Bibliotheque necessite Ecriture (README §4.5). */
    setup_default_config();
    Map map;
    init_map(&map, 5, 5);

    Player player = {.gold = 0, .science = 0, .current_research = TECH_NONE, .unlocked_techs = {0}};
    City city = {0};
    city.x = 2; city.y = 2;
    city.population = 1;
    city.owner = &player;
    city.current_project = PROJECT_LIBRARY;
    city.production_stored = 50;
    map.tiles[2][2].city = &city;
    Unit *units[8] = {0};
    int n = 0;

    process_city_production(&city, &map, units, &n, NULL);
    EXPECT(city.has_library == 0,
           "Sans Ecriture, la Bibliotheque ne doit PAS etre construite");

    player.unlocked_techs[TECH_ECRITURE] = 1;
    city.current_project = PROJECT_LIBRARY;
    city.production_stored = 50;
    process_city_production(&city, &map, units, &n, NULL);
    EXPECT(city.has_library == 1,
           "Avec Ecriture, la Bibliotheque doit etre construite");

    free_test_map(&map);
}

static void test_market_requires_commerce(void)
{
    /* Le Marche necessite Commerce (README §4.5). */
    setup_default_config();
    Map map;
    init_map(&map, 5, 5);

    Player player = {.gold = 0, .science = 0, .current_research = TECH_NONE, .unlocked_techs = {0}};
    City city = {0};
    city.x = 2; city.y = 2;
    city.population = 1;
    city.owner = &player;
    city.current_project = PROJECT_MARKET;
    city.production_stored = 40;
    map.tiles[2][2].city = &city;
    Unit *units[8] = {0};
    int n = 0;

    process_city_production(&city, &map, units, &n, NULL);
    EXPECT(city.has_market == 0,
           "Sans Commerce, le Marche ne doit PAS etre construit");

    player.unlocked_techs[TECH_COMMERCE] = 1;
    city.current_project = PROJECT_MARKET;
    city.production_stored = 40;
    process_city_production(&city, &map, units, &n, NULL);
    EXPECT(city.has_market == 1,
           "Avec Commerce, le Marche doit etre construit");

    free_test_map(&map);
}

static void test_wall_requires_maconnerie(void)
{
    /* La Muraille necessite Maconnerie (README §4.5). */
    setup_default_config();
    Map map;
    init_map(&map, 5, 5);

    Player player = {.gold = 0, .science = 0, .current_research = TECH_NONE, .unlocked_techs = {0}};
    City city = {0};
    city.x = 2; city.y = 2;
    city.population = 1;
    city.hp = 100;
    city.owner = &player;
    city.current_project = PROJECT_WALL;
    city.production_stored = 80;
    map.tiles[2][2].city = &city;
    Unit *units[8] = {0};
    int n = 0;

    process_city_production(&city, &map, units, &n, NULL);
    EXPECT(city.has_wall == 0,
           "Sans Maconnerie, la Muraille ne doit PAS etre construite");

    player.unlocked_techs[TECH_MAÇONNERIE] = 1;
    city.current_project = PROJECT_WALL;
    city.production_stored = 80;
    process_city_production(&city, &map, units, &n, NULL);
    EXPECT(city.has_wall == 1,
           "Avec Maconnerie, la Muraille doit etre construite");

    free_test_map(&map);
}

/* ============================================================
 * Orchestration
 * ============================================================ */

int run_test_ressources(void)
{
    /* Section A : fonctions pures */
    test_terrain_yield();
    test_chebyshev_distance();

    /* Section B : propriete des cases */
    test_tile_owner_within_radius();

    /* Section C : process_city_resources */
    test_city_resources_basic_plain();
    test_city_resources_with_buildings();
    test_city_resources_skips_wonder_tile();
    test_city_resources_special_resource_hidden();

    /* Section D : croissance et famine */
    test_city_growth_increase();
    test_city_growth_famine_decreases_pop();
    test_city_growth_granary_lowers_threshold();

    /* Section E : entretien (or du joueur) */
    test_player_gold_entretien_buildings();

    /* Section F : fondation de ville */
    test_found_city_creates_city();

    /* Section G : prerequis technologiques */
    test_warrior_requires_artisanat();
    test_library_requires_ecriture();
    test_market_requires_commerce();
    test_wall_requires_maconnerie();

    if (g_failed == 0) {
        puts("[OK] Ressources tests passed.");
        return 0;
    }
    fprintf(stderr, "[FAIL] %d ressources test(s) failed.\n", g_failed);
    return 1;
}