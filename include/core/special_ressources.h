#ifndef CORE_SPECIAL_RESOURCES_H
#define CORE_SPECIAL_RESOURCES_H

#include "types.h"

/* ============================================================
 * EXT-2 : Ressources spéciales
 * ----------------------------------------------------------------
 * Environ 15% des tuiles éligibles portent une ressource spéciale.
 * Chaque ressource :
 *   - est compatible avec 1 ou 2 types de terrain
 *   - apporte un bonus de yield (food, prod, gold, science ou PM)
 *   - nécessite une technologie débloquée pour produire son bonus
 *   - n'est VISIBLE sur la carte que si la techno est débloquée
 * ============================================================ */

#define SPRES_MAX_VALID_TERRAINS 2

typedef struct SpecialResourceDef {
    SpecialResourceType id;
    const char *name;
    TerrainType valid_terrains[SPRES_MAX_VALID_TERRAINS];
    int num_valid_terrains;
    TechType required_tech;
    int bonus_food;
    int bonus_prod;
    int bonus_gold;
    int bonus_science;
    int bonus_pm;       /* Chevaux uniquement : +1 PM aux unités produites */
    char ascii_char;    /* symbole en mode CLI */
} SpecialResourceDef;

/* Catalogue. Indexé par SpecialResourceType. */
extern const SpecialResourceDef SPRES_CATALOG[NUM_SPECIAL_RESOURCES];

/* ---------- Helpers ---------- */

/* Renvoie 1 si le terrain peut accueillir la ressource. */
int sres_terrain_compatible(SpecialResourceType sr, TerrainType t);

/* Renvoie 1 si la ressource est visible/utilisable par le joueur
 * (techno requise débloquée). */
int sres_visible(const Player *p, SpecialResourceType sr);

/* Pointe vers la définition (NULL si invalide). */
const SpecialResourceDef *sres_get(SpecialResourceType sr);

/* Tirage aléatoire d'une ressource compatible avec le terrain.
 * Renvoie SPRES_NONE si aucune ressource ne peut être placée sur ce
 * terrain. Utilise rand() : la graine doit être initialisée avant. */
SpecialResourceType sres_random_for_terrain(TerrainType t);

/* Calcule le bonus de yield (food/prod/gold/science) qu'apporte la
 * ressource à une ville à portée, SI la techno requise est débloquée.
 * Met à 0 sinon. N'inclut PAS le bonus PM (qui se gère différemment). */
TileYield sres_yield_for_player(SpecialResourceType sr, const Player *p);

#endif /* CORE_SPECIAL_RESOURCES_H */