#include <stdio.h>
#include "core/types.h"
#include "core/physics.h"

int main() {
    printf("--- TEST DE LA PHYSIQUE ---\n");

    // 1. Création de faux joueurs (Mock)
    Player joueur1 = {100, 0};
    Player barbare = {0, 0};

    // 2. Création de fausses unités (Mock)
    Unit mon_guerrier = {UNIT_WARRIOR, 15, 15, 3, 2, 3, 3, 0, 0, &joueur1};
    Unit ennemi = {UNIT_BARBARIAN, 15, 15, 3, 2, 1, 1, 1, 1, &barbare};
    // 3. Création de fausses tuiles (Mock)
    Tile case_montagne = {TERRAIN_MONTAGNE, NULL, NULL};
    Tile case_plaine = {TERRAIN_PLAINE, NULL, &ennemi};

    // TEST MOUVEMENT
    printf("Peut bouger sur Montagne ? %s\n", can_move(&mon_guerrier, &case_montagne) ? "OUI" : "NON");
    
    // TEST COMBAT
    printf("\nAvant combat :\n");
    printf("Guerrier PV: %d | Barbare PV: %d\n", mon_guerrier.hp, ennemi.hp);
    
    resolve_combat(&mon_guerrier, &ennemi);
    
    printf("Apres combat :\n");
    printf("Guerrier PV: %d | Barbare PV: %d\n", mon_guerrier.hp, ennemi.hp);

    return 0;
}