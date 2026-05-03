#include "core/physics.h"
#include <stddef.h> // Pour NULL

//1. coût des déplacements selon le terrain
int get_terrain_pm_cost(TerrainType type) {
    switch (type) {
        case TERRAIN_PLAINE:
        case TERRAIN_DESERT:
        case TERRAIN_TOUNDRA:
            return 1; // Coût de déplacement pour les plaines, déserts et toundras
        case TERRAIN_FORET:
            return 2; // Coût de déplacement pour les forêts
        case TERRAIN_MONTAGNE:
            return 3; // Coût de déplacement pour les montagnes
        case TERRAIN_EAU:
            return 999; // Coût de déplacement pour l'eau (impossible à traverser) si on n'a pas la tech bateau
        default:
            return 1; // Par défaut, coût de déplacement de 1
    }

}

//2. Vérification de la possibilité de déplacement
int can_move(Unit *unit, Tile *destination, Player *player) {
    if (unit == NULL || destination == NULL) {
        return 0; // Impossible de déplacer si l'unité ou la destination est NULL
    }
    int cost = get_terrain_pm_cost(destination->type);

    // Logique de Navigation maritime (Technologie bateau)
    // Si la cible est de l'eau, on vérifie l'arbre technologique du joueur
    if (destination->type == TERRAIN_EAU) {
        // Vérification de la technologie bateau
        if (player != NULL && !player->unlocked_techs[TECH_BATEAU]) {
            cost = 2; // Le cout passe à 2PM au lieu de 999
        }
    }

    // L'unité a t'elle assez de PM pour se déplacer sur ce terrain ?
    if (unit->pm < cost) {
        return 0; // Impossible de déplacer si l'unité n'a pas assez de PM
    }

    if(cost == 999){
        return 0; // Impossible de déplacer si le terrain est de l'eau
    }

    //Y a-t-il déjà une unité sur la tuile de destination ?
    if (destination->unit != NULL) {
        //on ne peut pas s'empiler sur un alié 
        if (destination->unit->owner == unit->owner) {
            return 0; // Impossible de déplacer si une unité alliée est déjà présente
        }

        // Si c'est un ennemi, c'est valide (ça déclenche un combat)
    }

    return 1; // Déplacement valide
}

//3. Résolution des combats
int max(int a, int b) {
    return (a > b) ? a : b;
}

void resolve_combat(Unit *attacker, Unit *defender) {
    if (attacker == NULL || defender == NULL) {
        return; // Impossible de résoudre le combat si l'attaquant ou le défenseur est NULL
    }

    // Un Colon (attaque 0) ne peut pas infliger de dégâts
    if (attacker->attack == 0) return;

    // Calcul des dégâts
    int dmg_to_defender = max(1, attacker->attack - defender->defense);
    int dmg_to_attacker = max(0, defender->attack - attacker->defense);

    // Application des dégâts
    defender->hp -= dmg_to_defender;
    attacker->hp -= dmg_to_attacker;

    // Empêcher les PV négatifs
    if (defender->hp < 0) defender->hp = 0;
    if (attacker->hp < 0) attacker->hp = 0;
}

// 4. Résolution des combats contre les Villes

void resolve_combat_city(Unit *attacker, City *city) {
    if (attacker == NULL || city == NULL) {
        return;
    }

    if (attacker->attack == 0) return; // Un colon ne peut pas attaquer une ville

    // Règle du Readme : Force du bâtiment = 20(30 si muraille)
    int city_strength = city->has_wall ? 30 : 20;

    // Calcul des dégâts
    int dmg_to_city = max(1, attacker->attack - city_strength);
    int dmg_to_attacker = max(0, city_strength - attacker->defense);

    // Application des dégâts
    city->hp -= dmg_to_city;
    attacker->hp -= dmg_to_attacker;

    // Empêcher les PV négatifs
    if (city->hp < 0) city->hp = 0;
    if (attacker->hp < 0) attacker->hp = 0;
}
