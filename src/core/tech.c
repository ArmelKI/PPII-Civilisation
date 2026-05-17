#include "../../include/core/game.h"
#include "../../include/core/wonder.h"
#include <stdio.h>


// Le catalogue en dur conformément au readme

const TechnologyDef TECH_CATALOG[MAX_TECHNOLOGIES] = {
    {TECH_CHASSE,       "Chasse",             50 ,  TECH_NONE,         TECH_NONE},
    {TECH_AGRICULTURE,  "Agriculture",        60,   TECH_NONE,         TECH_NONE},
    {TECH_ARTISANAT,    "Artisanat",          70,   TECH_NONE,         TECH_NONE},
    {TECH_ECRITURE,     "Écriture",           80,   TECH_NONE,         TECH_NONE},
    {TECH_EQUITATION,   "Équitation",         100,  TECH_NONE,         TECH_NONE},
    {TECH_IRRIGATION,   "Irrigation",         90,   TECH_AGRICULTURE,  TECH_NONE},
    {TECH_MAÇONNERIE,   "Maçonnerie",         100,  TECH_ARTISANAT,    TECH_NONE},
    {TECH_COMMERCE,     "Commerce",            90,  TECH_ARTISANAT,    TECH_NONE},
    {TECH_BATEAU,       "Bateau",              110, TECH_COMMERCE,     TECH_NONE},
};

//Vérifie si une technologie peut être recherchée par le joueur (en fonction de ce qu'il a déjà débloqué)

int can_research(Player *p, TechType tech) {
    if (tech < 0 || tech >= MAX_TECHNOLOGIES) {
        return 0; // Technologie invalide
    }

    //Déja débloquée
    if (p->unlocked_techs[tech]) {
        return 0;
    }

    const TechnologyDef *def = &TECH_CATALOG[tech];

    // Vérification du prérequis 1
    if (def->req1 != TECH_NONE && !p->unlocked_techs[def->req1]) {
        return 0; // Prérequis 1 non satisfait
    }
    //Vérification du prérequis 2
    if (def->req2 != TECH_NONE && !p->unlocked_techs[def->req2]) {
        return 0; // Prérequis 2 non satisfait
    }

    return 1; // Tous les prérequis sont satisfaits
}


// Fonction à appeler quand le joueur choisit une nouvelle recherche
void start_research(Player *p, TechType tech) {
    if (!can_research(p, tech)) {
        return;
    }

    // Règle du Readme : changer en cours de route vide la cagnotte de moitié
    if (p->current_research != TECH_NONE && p->current_research != tech) {
        p->science /= 2;  // Division entière par 2
    }

    p->current_research = tech;
}


// Fonction à appeler à chaque FIN DE TOUR pour vérifier si on a trouvé
void update_research(Player *p, const GameState *game) {
    if (p->current_research == TECH_NONE) return; // Pas de recherche en cours

    TechType tech = p->current_research;
    int cost = TECH_CATALOG[tech].cost;

    /* la Tour de Babel applique -25% sur le coût des futures techs.
     * Le helper renvoie 1.0 si Babel n'est pas construite ou si game == NULL,
     * donc l'ancien comportement est préservé. */
    float mult = wonder_tech_cost_multiplier(game);
    if (mult != 1.0f) {
        cost = (int)(cost * mult + 0.5f);
        if (cost < 1) cost = 1;
    }

    if(p->science >= cost) {
        // Technologie débloquée !
        p->unlocked_techs[tech] = 1;

        //Règle du Readme: la cagnotte est remise à zéro(surplus perdu)
        p->science = 0;
        p->current_research = TECH_NONE;
    }
}
