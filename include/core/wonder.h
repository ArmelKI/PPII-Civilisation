#ifndef CORE_WONDERS_H
#define CORE_WONDERS_H

#include "types.h"

/* ============================================================
 * EXT-2 : Merveilles du Monde
 * ----------------------------------------------------------------
 * Une Merveille se construit comme un bâtiment (production stockée
 * dans une ville) mais elle est UNIQUE dans toute la partie et son
 * bonus est GLOBAL. Elle occupe une case du rayon d'exploitation
 * de la ville, sur un terrain compatible, et la case ne produit
 * plus rien tant que la merveille existe.
 * ============================================================ */

/* Déclaration anticipée pour éviter une inclusion circulaire avec game.h */
struct GameState;
typedef struct GameState GameState;

typedef struct WonderDef {
    WonderType id;
    const char *name;           // nom affiché
    int cost;                   // coût en Production
    TerrainType terrain;        // terrain requis pour la case d'accueil
    ProjectType project;        // ProjectType correspondant (pour le menu de la ville)
    const char *effect_desc;    // courte description de l'effet (pour l'UI)
} WonderDef;

/* Catalogue en dur des 5 merveilles. Indexé par WonderType. */
extern const WonderDef WONDER_CATALOG[NUM_WONDERS];

/* ---------- Helpers de catalogue (purs, sans état) ---------- */

/* Renvoie 1 si le terrain est compatible avec la merveille demandée. */
int wonder_terrain_compatible(WonderType w, TerrainType t);

/* Conversions entre WonderType et ProjectType. WONDER_NONE / PROJECT_NONE
 * en cas d'absence de correspondance. */
ProjectType wonder_to_project(WonderType w);
WonderType project_to_wonder(ProjectType p);

/* Renvoie le coût de production de la merveille (0 si invalide). */
int wonder_cost(WonderType w);

/* Renvoie le nom affichable (chaîne statique, ne pas free). */
const char *wonder_name(WonderType w);

/* ---------- Helpers qui dépendent du GameState ---------- */

/* 1 si la merveille a déjà été construite par quelqu'un. */
int wonder_is_built(const GameState *g, WonderType w);

/* Cherche dans le rayon d'exploitation d'une ville une case libre du
 * terrain requis pour héberger la merveille. Renvoie 1 si trouvée,
 * 0 sinon. Si trouvée, *out_x et *out_y reçoivent les coordonnées.
 * Une case est "libre" si elle n'a pas déjà de merveille ET n'est
 * pas la case centrale de la ville (où il y a déjà un bâtiment).
 * NB : on tolère qu'une unité passe dessus (la merveille n'en est
 * pas affectée). */
int wonder_can_build_in_city(const GameState *g, const City *city,
                             WonderType w, int *out_x, int *out_y);

/* Marque la merveille comme construite à la position (x, y) hébergée
 * par la ville city_id. Met aussi à jour la tuile correspondante.
 * Aucune vérification : à appeler après wonder_can_build_in_city(). */
void wonder_mark_built(GameState *g, WonderType w, int x, int y, int city_id);

/* Détruit la merveille w : libère la case, remet l'état global à 0.
 * Utilisé quand la ville hôte est rasée. */
void wonder_destroy(GameState *g, WonderType w);

/* À appeler quand une ville est détruite (rasée par les barbares ou
 * dissoute par famine). Détruit toutes les merveilles que la ville
 * hébergeait. */
void wonder_handle_city_destroyed(GameState *g, int city_id);

/* ---------- Modificateurs d'effets globaux ----------
 * Ces helpers sont consommés par ressources.c, tech.c, etc. Ils
 * renvoient des valeurs "neutres" (1.0 multiplicateur, 0 bonus)
 * quand la merveille n'est pas construite. */

/* Pyramides : 1.5 si construite, sinon 1.0 */
float wonder_food_multiplier(const GameState *g);

/* Jardin suspendu : 0.8 si construit, sinon 1.0 */
float wonder_growth_threshold_multiplier(const GameState *g);

/* Tour de Babel : 0.75 si construite, sinon 1.0 */
float wonder_tech_cost_multiplier(const GameState *g);

/* Colisée : 2 par ville si construit, sinon 0. À appliquer une fois
 * par ville par tour (typiquement dans process_city_resources). */
int wonder_extra_gold_per_city(const GameState *g);

/* Grande Bibliothèque : à appeler juste après sa construction pour
 * débloquer immédiatement la techno en cours sans toucher à la
 * cagnotte de science accumulée. */
void wonder_apply_great_library_effect(GameState *g);

#endif /* CORE_WONDERS_H */