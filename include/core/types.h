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
    TERRAIN_TOUNDRA,
} TerrainType;

/*enum pour dire qu'un terrain est dans cette zone et cette zone uniquement, utile pour l'affichage et pour 
notre code*/ 

/* 2. DÉCLARATION ANTICIPÉES*/


/* Ces structs seront utilisés plus bas, on déclare pour prévenir le compilateur*/

typedef struct City City;
typedef struct Unit Unit;
typedef struct Player Player;
typedef struct BarbarianCamp BarbarianCamp;


//Énumération des technologies disponibles
typedef enum {
    TECH_NONE = -1,  //Aucune recherche en cours
    TECH_CHASSE = 0,
    TECH_AGRICULTURE = 1,
    TECH_ARTISANAT = 2,
    TECH_ECRITURE = 3,
    TECH_EQUITATION = 4,
    TECH_IRRIGATION = 5,
    TECH_MAÇONNERIE = 6,
    TECH_COMMERCE = 7,
    TECH_BATEAU = 8,    //Ajout de la technologie bateau
    MAX_TECHNOLOGIES = 9,
} TechType;

// Structure pour définir une technologie dans le catalogue
typedef struct {
    TechType id;                  // Identifiant de la technologie
    const char* name;             // Nom de la technologie
    int cost;                     // Coût en science pour débloquer la technologie
    TechType req1;             // Première technologie prérequise (TECH_NONE si aucune)
    TechType req2;             // Seconde technologie prérequise (Pour commerce notamment)
} TechnologyDef;

/* 3. STRUCTURES DE LA CARTE  */
typedef enum{
    FOG_HIDDEN = 0, // Noir total(jamais exploré)
    FOG_EXPLORED = 1, // Grise (déja visité, on se souvient du terrain)
    FOG_VISIBLE = 2, // Éclairé(champ de vision actuel d'une unité/ville)
} FogState;

typedef struct Tile{
    TerrainType type; // Type de terrain (Plaine, eau ..)
    FogState visibility; // État de visibilité de la tuile (inexplorée, explorée, visible) 
    City* city;      // Pointeur vers la ville présente sur ce terrain (NULL si aucune)
    Unit* unit;      // Pointeur vers l'unité présente sur ce terrain (NULL si aucune)
    BarbarianCamp* camp; // Pointeur vers le camp de barbares présent sur ce terrain (NULL si aucun)
} Tile;

typedef struct Map{
    int width;       // Largeur de la carte
    int height;      // Hauteur de la carte
    Tile** tiles;    // Grille de tuiles (2D array)

    int start_x;     // Coordonnées de la ville de départ
    int start_y;
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

    // Gestion des technologies
    TechType current_research; // Technologie en cours de recherche
    int unlocked_techs[MAX_TECHNOLOGIES]; // Tableau indiquant les technologies débloquées (1) ou non (0)

} Player;

typedef enum {
    PROJECT_NONE = 0,   // Rien en cours
    PROJECT_SETTLER,    // Colon (50 prod)
    PROJECT_WARRIOR,    // Guerrier (40 prod)
    PROJECT_ARCHER,     // Archer (55 prod)
    PROJECT_GRANARY,    // Grenier (30 prod)
    PROJECT_WORKSHOP,   // Atelier (40 prod)
    PROJECT_LIBRARY,    // Bibliothèque (50 prod)
    PROJECT_MARKET,     // Marché (40 prod)
    PROJECT_BARRACKS,   // Caserne (60 prod)
    PROJECT_WALL        // Muraille (80 prod)
} ProjectType;

typedef struct City {
    int id;                     // id unique repère d'ancienetté 0 pour la première +1 à chaque nouvelle ville
    int population;             // Nombre d'habitants
    int food_stored;            // Nourriture stockée pour faire agrandir la population
    int production_stored;      // Production stockée pour le projet en cours
    int hp;                     // Points de vie

    int has_wall;              // Indique si la ville a une muraille (1) ou pas (0)
    int has_granary;           // Grenier
    int has_workshop;          // Atelier
    int has_library;           // Bibliothèque
    int has_market;            // Marché
    int has_barrack;           // caserne

    ProjectType current_project;
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
    UNIT_ARCHER,         //L'archer du joueur(Militaire a distance simplifie)
    UNIT_BARBARIAN,       //Le Guerrier barbare (Ennemi contrôlé par le jeu)
} UnitType;

// Structure représentant une unité sur la carte

typedef struct Unit {
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

/* 5. L'ARBRE TECHNOLOGIQUE(Graphe Orienté)*/


// structure d'une technologie dans l'arbre technologique



typedef struct {
    int food;
    int production;
    int gold;
    int science;
} TileYield;             //permet de définir et manipuler le rendement d'une case







#endif // CORE_TYPES_H
