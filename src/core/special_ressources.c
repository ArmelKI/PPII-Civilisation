#include "../../include/core/special_ressources.h"

#include <stdlib.h>

/* Catalogue en dur. L'ordre DOIT correspondre à SpecialResourceType. */
const SpecialResourceDef SPRES_CATALOG[NUM_SPECIAL_RESOURCES] = {
    /* id              name        terrains valides              n  tech           f  p  g  s  pm  char */
    { SPRES_WHEAT,     "Ble",      {TERRAIN_PLAINE, 0},          1, TECH_AGRICULTURE, 3, 0, 0, 0, 0, 'b' },
    { SPRES_IRON,      "Fer",      {TERRAIN_MONTAGNE, 0},        1, TECH_ARTISANAT,   0, 3, 0, 0, 0, 'i' },
    { SPRES_SPICES,    "Epices",   {TERRAIN_FORET, TERRAIN_DESERT}, 2, TECH_COMMERCE, 0, 0, 4, 0, 0, 'e' },
    { SPRES_SPECIMENS, "Specimens",{TERRAIN_TOUNDRA, 0},         1, TECH_ECRITURE,    0, 0, 0, 3, 0, 's' },
    { SPRES_HORSES,    "Chevaux",  {TERRAIN_PLAINE, TERRAIN_TOUNDRA}, 2, TECH_EQUITATION, 0, 0, 0, 0, 1, 'h' }
};

int sres_terrain_compatible(SpecialResourceType sr, TerrainType t) {
    if (sr < 0 || sr >= NUM_SPECIAL_RESOURCES) return 0;
    const SpecialResourceDef *def = &SPRES_CATALOG[sr];
    for (int i = 0; i < def->num_valid_terrains; i++) {
        if (def->valid_terrains[i] == t) return 1;
    }
    return 0;
}

int sres_visible(const Player *p, SpecialResourceType sr) {
    if (!p || sr < 0 || sr >= NUM_SPECIAL_RESOURCES) return 0;
    TechType req = SPRES_CATALOG[sr].required_tech;
    if (req < 0 || req >= MAX_TECHNOLOGIES) return 0;
    return p->unlocked_techs[req] != 0;
}

const SpecialResourceDef *sres_get(SpecialResourceType sr) {
    if (sr < 0 || sr >= NUM_SPECIAL_RESOURCES) return NULL;
    return &SPRES_CATALOG[sr];
}

SpecialResourceType sres_random_for_terrain(TerrainType t) {
    /* On construit la liste des candidats compatibles puis on tire au sort. */
    SpecialResourceType candidates[NUM_SPECIAL_RESOURCES];
    int n = 0;
    for (int i = 0; i < NUM_SPECIAL_RESOURCES; i++) {
        if (sres_terrain_compatible((SpecialResourceType)i, t)) {
            candidates[n++] = (SpecialResourceType)i;
        }
    }
    if (n == 0) return SPRES_NONE;
    return candidates[rand() % n];
}

TileYield sres_yield_for_player(SpecialResourceType sr, const Player *p) {
    TileYield y = {0, 0, 0, 0};
    if (sr < 0 || sr >= NUM_SPECIAL_RESOURCES) return y;
    if (!sres_visible(p, sr)) return y;

    const SpecialResourceDef *def = &SPRES_CATALOG[sr];
    y.food = def->bonus_food;
    y.production = def->bonus_prod;
    y.gold = def->bonus_gold;
    y.science = def->bonus_science;
    return y;
}