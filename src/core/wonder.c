#include "../../include/core/wonder.h"
#include "../../include/core/game.h"
#include "../../include/core/tech.h"

#include <stddef.h>



const WonderDef WONDER_CATALOG[NUM_WONDERS] = {
    { WONDER_PYRAMIDS,        "Pyramides",         200, TERRAIN_DESERT,   PROJECT_WONDER_PYRAMIDS,       "+50% Nourriture toutes villes" },
    { WONDER_GREAT_LIBRARY,   "Grande Bibliotheque",250, TERRAIN_TOUNDRA,  PROJECT_WONDER_GREAT_LIBRARY,  "Offre immediatement la techno en cours" },
    { WONDER_COLISEUM,        "Colisee",           180, TERRAIN_PLAINE,   PROJECT_WONDER_COLISEUM,       "+2 Or/tour par ville" },
    { WONDER_BABEL,           "Tour de Babel",     300, TERRAIN_MONTAGNE, PROJECT_WONDER_BABEL,          "-25% cout futures technos" },
    { WONDER_HANGING_GARDEN,  "Jardins suspendus", 220, TERRAIN_FORET,    PROJECT_WONDER_HANGING_GARDEN, "-20% seuil de croissance" }
};

/* ---------- Helpers purs ---------- */

int wonder_terrain_compatible(WonderType w, TerrainType t) {
    if (w < 0 || w >= NUM_WONDERS) return 0;
    return WONDER_CATALOG[w].terrain == t;
}

ProjectType wonder_to_project(WonderType w) {
    if (w < 0 || w >= NUM_WONDERS) return PROJECT_NONE;
    return WONDER_CATALOG[w].project;
}

WonderType project_to_wonder(ProjectType p) {
    for (int i = 0; i < NUM_WONDERS; i++) {
        if (WONDER_CATALOG[i].project == p) return (WonderType)i;
    }
    return WONDER_NONE;
}

int wonder_cost(WonderType w) {
    if (w < 0 || w >= NUM_WONDERS) return 0;
    return WONDER_CATALOG[w].cost;
}

const char *wonder_name(WonderType w) {
    if (w < 0 || w >= NUM_WONDERS) return "?";
    return WONDER_CATALOG[w].name;
}

/* ---------- Helpers dépendants du GameState ---------- */

int wonder_is_built(const GameState *g, WonderType w) {
    if (!g || w < 0 || w >= NUM_WONDERS) return 0;
    return g->wonder_built[w] != 0;
}

/* Calcule le rayon d'exploitation d'une ville selon sa population.
 * Recopie la règle de ressources.c pour rester cohérent. */
static int city_radius(const City *c) {
    if (c->population >= 26) return 3;
    if (c->population >= 10) return 2;
    return 1;
}

int wonder_can_build_in_city(const GameState *g, const City *city,
                             WonderType w, int *out_x, int *out_y) {
    if (!g || !city || w < 0 || w >= NUM_WONDERS) return 0;
    if (wonder_is_built(g, w)) return 0;

    const WonderDef *def = &WONDER_CATALOG[w];
    int rayon = city_radius(city);

    for (int dy = -rayon; dy <= rayon; dy++) {
        for (int dx = -rayon; dx <= rayon; dx++) {
            int tx = city->x + dx;
            int ty = city->y + dy;
            if (tx < 0 || tx >= g->map.width || ty < 0 || ty >= g->map.height) {
                continue;
            }
            /* On ne pose pas la merveille sur la case centrale de la ville. */
            if (dx == 0 && dy == 0) continue;

            const Tile *t = &g->map.tiles[ty][tx];
            if (t->type != def->terrain) continue;
            if (t->wonder != WONDER_NONE) continue;
            /* On évite de poser sur une autre ville ou un campement. */
            if (t->city != NULL || t->camp != NULL) continue;

            if (out_x) *out_x = tx;
            if (out_y) *out_y = ty;
            return 1;
        }
    }
    return 0;
}

void wonder_mark_built(GameState *g, WonderType w, int x, int y, int city_id) {
    if (!g || w < 0 || w >= NUM_WONDERS) return;
    g->wonder_built[w] = 1;
    g->wonder_x[w] = x;
    g->wonder_y[w] = y;
    g->wonder_host_city_id[w] = city_id;
    if (y >= 0 && y < g->map.height && x >= 0 && x < g->map.width) {
        g->map.tiles[y][x].wonder = w;
    }
}

void wonder_destroy(GameState *g, WonderType w) {
    if (!g || w < 0 || w >= NUM_WONDERS) return;
    if (!g->wonder_built[w]) return;

    int x = g->wonder_x[w];
    int y = g->wonder_y[w];
    if (y >= 0 && y < g->map.height && x >= 0 && x < g->map.width) {
        g->map.tiles[y][x].wonder = WONDER_NONE;
    }
    g->wonder_built[w] = 0;
    g->wonder_x[w] = 0;
    g->wonder_y[w] = 0;
    g->wonder_host_city_id[w] = -1;
}

void wonder_handle_city_destroyed(GameState *g, int city_id) {
    if (!g) return;
    for (int w = 0; w < NUM_WONDERS; w++) {
        if (g->wonder_built[w] && g->wonder_host_city_id[w] == city_id) {
            wonder_destroy(g, (WonderType)w);
        }
    }
}

/* ---------- Modificateurs d'effets globaux ---------- */

float wonder_food_multiplier(const GameState *g) {
    return wonder_is_built(g, WONDER_PYRAMIDS) ? 1.5f : 1.0f;
}

float wonder_growth_threshold_multiplier(const GameState *g) {
    return wonder_is_built(g, WONDER_HANGING_GARDEN) ? 0.8f : 1.0f;
}

float wonder_tech_cost_multiplier(const GameState *g) {
    return wonder_is_built(g, WONDER_BABEL) ? 0.75f : 1.0f;
}

int wonder_extra_gold_per_city(const GameState *g) {
    return wonder_is_built(g, WONDER_COLISEUM) ? 2 : 0;
}

void wonder_apply_great_library_effect(GameState *g) {
    if (!g) return;
    Player *p = &g->player;
    TechType current = p->current_research;
    if (current == TECH_NONE) return;
    if (current < 0 || current >= MAX_TECHNOLOGIES) return;

    /* Règle README EXT-2 : on débloque immédiatement la techno en
     * cours TOUT EN CONSERVANT la science accumulée. */
    int saved_science = p->science;
    p->unlocked_techs[current] = 1;
    p->current_research = TECH_NONE;
    p->science = saved_science;
}
