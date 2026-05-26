#ifndef CORE_TYPES_H
#define CORE_TYPES_H

/* 1. Énumération de base*/


// Les 6 types de terrains //
typedef enum {
    TERRAIN_PLAINE,
    TERRAIN_FORET,
    TERRAIN_MONTAGNE,
    TERRAIN_EAU,
    TERRAIN_DESERT,
    TERRAIN_VILLE
} TerrainType;

/*enum pour dire qu'un terrain est dans cette zone et cette zone uniquement, utilse pour l'affichage et pour 
notre code*/ 

/* 2. DÉCLARATION ANTICIPÉES*/


/* Ces structs seront utilisés plus bas, on déclare pour prévenir le compilateur*/

typedef struct City City;
typedef struct Unit Unit;
typedef struct Player Player;
typedef struct BarbarianCamp BarbarianCamp;


/* 3. STRUCTURES DE LA CARTE  */

typedef struct Tile{
    TerrainType type; // Type de terrain (Plaine, eau ..)
    City* city;      // Pointeur vers la ville présente sur ce terrain (NULL si aucune)
    Unit* unit;      // Pointeur vers l'unité présente sur ce terrain (NULL si aucune)
} Tile;

typedef struct Map{
    int width;       // Largeur de la carte
    int height;      // Hauteur de la carte
    Tile** tiles;    // Grille de tuiles (2D array)
} Map;


/* CAMPEMENT BARBARE */

typedef struct BarbarianCamp {
    int x, y;                // Coordonnées du campement sur la carte
    int destroyed;           // 0 si actif, 1 si détruit
    int id;                  // Identifiant unique (pour déboguer et tracker)
} BarbarianCamp;


/* 4. LA VILLE ET LE JOUEUR */

typedef struct Player {
    int gold;
    int science;
    int camps_destroyed;

} Player;

typedef struct City {
    int id;                     // Identifiant unique pour les règles déterministes
    int population;             // Nombre d'habitants
    int food_stored;            // Nourriture stockée pour faire agrandir la population
    int production_stored;      // Production stockée pour le projet en cours
    int hp;                     // Points de vie
    int has_wall;              // Indique si la ville a une muraille (1) ou pas (0)

    // Coordonnées de la ville sur la carte
    int x;
    int y;

    Player *owner;                // Pointeur vers le joueur propriétaire de la ville
} City;


/* 5. LES UNITÉS ET LES BARBARES */

// Les différents types d'unités possibles
typedef enum {
    UNIT_SETTLER,       //Le colon (Fonde les villes, attaque 0, defense 1)
    UNIT_WARRIOR,        //Le guerrier du joueur(Militaire, attaque 3, defense 2)
    UNIT_BARBARIAN,       //Le Guerrier barbare (Ennemi contrôlé par le jeu)
} UnitType;

// Structure représentant une unité sur la carte

typedef struct Unit {
    int id;               // Identifiant unique pour les règles déterministes
    UnitType type;        // Type de l'unité (Colon, Guerrier du joueur, Barbarian)

    int hp;              // Points de vie de l'unité
    int hp_max;          // Points de vie maximum de l'unité

    int attack;          // Force d'attaque de l'unité
    int defense;         // Force de défense de l'unité

    int pm;              // Points de mouvement restants pour ce tour
    int pm_max;          // Points de mouvement maximum de l'unité

    // Coordonnées de l'unité sur la carte
    int x;
    int y;

    Player *owner;       // Pointeur vers le joueur propriétaire de l'unité (NULL pour les barbares)
} Unit;


#endif // CORE_TYPES_H
