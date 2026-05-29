#include <stdio.h>
#include "core/types.h"
#include "core/physics.h"

int run_test_physique(void) {
    printf("--- TEST DE LA PHYSIQUE ---\n");

    // 1. Création de faux joueurs (Mock) avec initialiseurs désignés
    Player joueur1 = {.gold = 100, .science = 0, .current_research = TECH_NONE, .unlocked_techs = {0}};
    Player barbare = {.gold = 0, .science = 0, .current_research = TECH_NONE, .unlocked_techs = {0}};

    // 2. Création de fausses unités (Mock) en utilisant des initialiseurs désignés
    Unit mon_guerrier = {.type = UNIT_WARRIOR, .hp = 15, .hp_max = 15, .attack = 3, .defense = 2, .pm = 3, .pm_max = 3, .x = 0, .y = 0, .owner = &joueur1};
    Unit ennemi = {.type = UNIT_BARBARIAN, .hp = 15, .hp_max = 15, .attack = 3, .defense = 2, .pm = 1, .pm_max = 1, .x = 1, .y = 0, .owner = &barbare};

    // 3. Création de fausses tuiles (Mock) avec champs explicités
    Tile case_montagne = {.type = TERRAIN_MONTAGNE, .visibility = FOG_VISIBLE, .city = NULL, .unit = NULL, .camp = NULL};

    // TEST MOUVEMENT: on passe le joueur en troisième argument conforme à can_move
    printf("Peut bouger sur Montagne ? %s\n", can_move(&mon_guerrier, &case_montagne, &joueur1) ? "OUI" : "NON");
    
    // TEST COMBAT
    printf("\nAvant combat :\n");
    printf("Guerrier PV: %d | Barbare PV: %d\n", mon_guerrier.hp, ennemi.hp);
    
    resolve_combat(&mon_guerrier, &ennemi);
    
    printf("Apres combat :\n");
    printf("Guerrier PV: %d | Barbare PV: %d\n", mon_guerrier.hp, ennemi.hp);

    return 0;
}