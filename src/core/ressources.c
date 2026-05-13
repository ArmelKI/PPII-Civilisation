#include <stdio.h>
#include <stdlib.h>
#include <ncurses.h>
#include "../../include/core/types.h"
#include "../../include/core/game.h"
#include "../../include/core/config.h"

// mise en place du rendement en fonction du type de case
TileYield get_terrain_yield(TerrainType type) {
    TileYield y = {0,0,0,0};

    if ( type == TERRAIN_PLAINE){

        y.food = 2;
        y.production = 1;

    } else if ( type == TERRAIN_FORET) {

        y.food = 1;
        y.production = 2;

    } else if ( type == TERRAIN_MONTAGNE) {

        y.production = 3;
        y.science = 1;

    } else if ( type ==  TERRAIN_EAU){

        y.food = 1;
        y.gold = 1;

    } else if ( type == TERRAIN_DESERT) {

        y.gold = 1;
    
    }

    return y;
}

//fonction distance de tchebychev
int get_chebyshev_distance(int x1, int y1, int x2, int y2) {

    int dx = abs(x1 - x2);
    int dy = abs(y1 - y2);
    return (dx > dy) ? dx : dy;

}

//fonction déterminant à quelle ville apaprtient une case
City* get_tile_owner(int target_x, int target_y, Map* map){

    City* best_city = NULL;
    int min_dist = 9999;

    for(int dy = -3; dy <= 3; dy++){
        for(int dx = -3; dx <= 3; dx++){

            int cx = target_x +dx;
            int cy = target_y +dy;

            if (cx >= 0 && cx < map->width && cy >= 0 && cy < map->height){

                City* potential_city = map -> tiles[cy][cx].city;

                if(potential_city != NULL){
                    
                    int dist = get_chebyshev_distance(cx, cy, target_x, target_y);

                    int rayon = 1;
                    if (potential_city->population >= 10 && potential_city->population <= 25) rayon = 2;
                    else if (potential_city->population > 25) rayon = 3;

                    if (dist <= rayon) {
                        if (dist < min_dist) {
                            best_city = potential_city;
                            min_dist = dist;
                        } else if (dist == min_dist){
                            if (best_city == NULL || potential_city->id < best_city->id) {
                                best_city = potential_city;
                            }
                        }
                    }
                }
            }
        }
    }
    return best_city;

}

//fonction qui actualise la ville à chaque tour
void process_city_resources(City *city, Map *map) {
    if (!city || !city->owner || !map) return;
    const GameConfig *config = game_config_get_active();
    float production_multiplier = config ? config->production_multiplier : 1.0f;

    //rayon d'exploitation de la ville
    int rayon = 1;
    if(city->population >= 10 && city->population <= 25) {
        rayon = 2;
    } else if (city->population > 25) {
        rayon = 3;
    }

    TileYield total_yield = {0, 0, 0, 0};

    //Parcourir le rayon autour de la ville pour mettre a jour les ressources générées
    for (int dy = -rayon; dy <= rayon; dy++) {
        for (int dx = -rayon; dx <= rayon; dx++) {

            int target_x = city->x + dx;
            int target_y = city->y + dy;
            
            if (target_x >= 0 && target_x < map->width && target_y >= 0 && target_y < map->height) {

                City* rightful_owner = get_tile_owner(target_x, target_y, map);

                if (rightful_owner == city) {
                    TerrainType t_type = map->tiles[target_y][target_x].type;
                    TileYield tile_y = get_terrain_yield(t_type);
                    
                    total_yield.food += tile_y.food;
                    total_yield.production += tile_y.production;
                    total_yield.gold += tile_y.gold;
                    total_yield.science += tile_y.science;
                }
            }
        }
    }

    if(city->has_granary) {
        total_yield.food += 3;
    }
    if(city->has_workshop) {
        total_yield.production += 3;
    }
    if(city->has_library) {
        total_yield.science += 4;
    }
    if(city->has_market) {
        total_yield.gold += 3;
    }

    int food_consumed = city->population * 2;       // consommation de nourriture
    
    // Maj des ressources de city et player
    city->food_stored += (total_yield.food - food_consumed);

    int scaled_production = (int)(total_yield.production * production_multiplier + 0.5f);
    if (total_yield.production > 0 && scaled_production < 1) {
        scaled_production = 1;
    }
    city->production_stored += scaled_production;
    
    city->owner->gold += total_yield.gold;
    city->owner->science += total_yield.science;

}


//fonction qui gère la croissance et la famine
void process_city_growth(City *city,Map *map){

    if (!city || !map) return;

    const GameConfig *config = game_config_get_active();
    float growth_multiplier = config ? config->growth_threshold_multiplier : 1.0f;
    int famine_enabled = !config || config->famine_enabled;

    int growth_seuil= 20 + (10* city -> population);

    if(city -> has_granary){
        growth_seuil = (growth_seuil*2)/3;
    }

    growth_seuil = (int)(growth_seuil * growth_multiplier + 0.5f);
    if (growth_seuil < 1) {
        growth_seuil = 1;
    }

    if( city -> food_stored >= growth_seuil){
        city -> population += 1;
        city -> food_stored = 0;
        int new_hp = 10 * city -> population;
        if (city -> has_wall){
            new_hp *= 2;
        }
        city -> hp = new_hp;
    }

    if( city -> food_stored < 0 && famine_enabled){

        city -> population -= 1; //demander aux autres si on remet a zero ou pas (ajuster si jeu trop difficile)
        city->food_stored = 0;

        if (city -> population <= 0){ //ville perdue
            map->tiles[city->y][city->x].city = NULL; // peu être modififier pour faire un free
            return;
        } else {
            int new_hp = 10 * city -> population;
            if (city -> has_wall){
                new_hp *= 2;
            }

            if (city->hp > new_hp){
                city -> hp = new_hp;
            }
        }
    } else if (city->food_stored < 0) {
        city->food_stored = 0;
    }
}

//fonction qui gère les dépenses en or et la réserve
void process_player_gold(Player *player, Map *map) {

    if (!player || !map) return;

    int total_maintenance = 0;

    Unit* player_units[1000];
    int unit_count = 0;

    for(int y=0; y < map -> height; y++){
        for(int x=0; x < map -> width; x++){

            Tile *tile = &(map -> tiles[y][x]);

            if (tile -> city != NULL && tile -> city -> owner == player){
                City *c = tile -> city;
                if (c->has_granary) total_maintenance += 1;
                if (c->has_workshop) total_maintenance += 1;
                if (c->has_library) total_maintenance += 1;
                if (c->has_market) total_maintenance += 1;
                if (c->has_barrack) total_maintenance += 2;
                if (c->has_wall) total_maintenance += 2;
            }

            if(tile->unit != NULL && tile->unit->owner == player) {
                Unit *u = tile->unit;

                player_units[unit_count] = u;
                unit_count++;

                if (u->type == UNIT_WARRIOR || u->type == UNIT_ARCHER || u->type == UNIT_CAVALIER) {
                    total_maintenance += 1;
                }

            }
        }
    }
    player -> gold -= total_maintenance;

    if(player -> gold < 0){
        printw("ALERTE BANQUEROUTE ! Decouvert de %d Or.\n", player->gold);

        if (unit_count > 0) {
            int random_index = rand() % unit_count;
            Unit *martyr_unit = player_units[random_index];

            printw("Une unite de type %d a ete licenciee pour des raisons economiques en [%d,%d]...\n",martyr_unit -> type, martyr_unit -> x, martyr_unit-> y );

            map->tiles[martyr_unit->y][martyr_unit->x].unit = NULL;

        } else {

            printw("Plus aucune unite a dissoudre, on constate une GREVE OUVRIERE, nos capacites ouvrieres son deux fois moins importantes...\n");

            for (int y=0; y < map -> height; y++){
                for (int x=0; x < map -> width; x++){
                    if (map->tiles[y][x].city != NULL && map->tiles[y][x].city->owner == player) {
                        map->tiles[y][x].city->production_stored /= 2;
                    }
                }
            }
        }
    }
}


//fondation d'une ville par un colon
int found_city(Unit *settler, Map *map, int *city_id_counter, City **player_cities, int *city_count){
    if(!settler||!map|| !city_id_counter) return 0;
    if(settler->type != UNIT_SETTLER) {
        printw("Seul un Colon peut fonder une ville !\n");
        return 0;
    }

    if (*city_count >= MAX_CITIES) {
        printw("Vous avez atteint la limite d'empire (%d villes max) !\n", MAX_CITIES);
        return 0;
    }

    int sx = settler -> x;
    int sy = settler -> y;
    Tile *target_tile = &(map->tiles[sy][sx]);

    if(target_tile->type == TERRAIN_EAU) {
        printw("Impossible de fonder une ville sur l'eau.\n");
        return 0;
    }
    if(target_tile->city != NULL) {
        printw("Une ville existe deja.\n");
        return 0;
    }

    for(int dy = -3; dy <= 3; dy++) {
        for (int dx = -3; dx <= 3; dx++) {
            int cx = sx + dx;
            int cy = sy + dy;
            
            if(cx >= 0 && cx < map->width && cy >= 0 && cy < map->height) {
                if (map->tiles[cy][cx].city != NULL) {
                    printw("Il existe deja une ville a moins de 4 cases.\n");
                    return 0; 
                }
            }
        }
    }


    //aucunde condition éliminatoire : fondation de la ville

    City *new_city = malloc(sizeof(City));
    if (!new_city) {
        printw("Erreur memoire lors de la creation de la ville.\n");
        return 0;
    }

    new_city->id = *city_id_counter; 
    (*city_id_counter)++;

    new_city->population = 1;
    new_city->food_stored = 0;
    new_city->production_stored = 0;
    new_city->hp = 10;
    new_city->x = sx;
    new_city->y = sy;
    new_city->owner = settler->owner;

    new_city->has_wall = 0;
    new_city->has_granary = 0;
    new_city->has_workshop = 0;
    new_city->has_library = 0;
    new_city->has_market = 0;
    new_city->has_barrack = 0;

    target_tile->city = new_city;

    player_cities[*city_count] = new_city;
    (*city_count)++;

    target_tile->unit = NULL;
    free(settler);

    printw("Une nouvelle ville vient d'etre fondee en [%d, %d] !\n", sx, sy);
    return 1;
}


//gestion de la production de bâtiments et d'unités
void process_city_production(City *city, Map *map, Unit **player_units, int *player_unit_count) {
    if (!city || !map) return;

    // pas de projet => la production est perdue !
    if (city->current_project == PROJECT_NONE) {
        city->production_stored = 0;
        return;
    }

    int cost = 0;
    switch (city->current_project) {
        case PROJECT_GRANARY:  cost = 30; break;
        case PROJECT_WORKSHOP: cost = 40; break;
        case PROJECT_LIBRARY:  cost = 50; break;
        case PROJECT_MARKET:   cost = 40; break;
        case PROJECT_BARRACKS: cost = 60; break;
        case PROJECT_WALL:     cost = 80; break;
        case PROJECT_SETTLER:  cost = 50; break;
        case PROJECT_WARRIOR:  cost = 40; break;
        case PROJECT_ARCHER:   cost = 55; break;
        case PROJECT_CAVALIER: cost = 60; break;
        default: return;
    }

    // Les unités militaires avancées exigent une caserne.
    if ((city->current_project == PROJECT_WARRIOR || city->current_project == PROJECT_ARCHER || city->current_project == PROJECT_CAVALIER) && !city->has_barrack) {
        return;
    }

    // L'archer nécessite la technologie Chasse.
    if (city->current_project == PROJECT_ARCHER &&
        (!city->owner || !city->owner->unlocked_techs[TECH_CHASSE])) {
        return;
    }

    // Le cavalier nécessite la technologie Équitation.
    if (city->current_project == PROJECT_CAVALIER &&
        (!city->owner || !city->owner->unlocked_techs[TECH_EQUITATION])) {
        return;
    }

    if (city->production_stored >= cost) {

        city->production_stored -= cost; 

        switch (city->current_project) {
            
            case PROJECT_GRANARY:  city->has_granary = 1;  printw("Grenier construit !\n"); break;
            case PROJECT_WORKSHOP: city->has_workshop = 1; printw("Atelier construit !\n"); break;
            case PROJECT_LIBRARY:  city->has_library = 1;  printw("Bibliotheque construite !\n"); break;
            case PROJECT_MARKET:   city->has_market = 1;   printw("Marche construit !\n"); break;
            case PROJECT_BARRACKS: city->has_barrack = 1; printw("Caserne construite !\n"); break;
            case PROJECT_WALL: {
                city->has_wall = 1;
                city->hp *= 2; 
                printw("Muraille construite !\n");
                break;
            }


            case PROJECT_WARRIOR:
            case PROJECT_ARCHER:
            case PROJECT_CAVALIER:
            case PROJECT_SETTLER: {
                // Sécurité : On vérifie qu'on n'a pas atteint la limite d'unités actives (8 max)
                if (*player_unit_count >= 8) {
                    printw("Limite d'unites actives atteinte (8 max), production en attente.\n");
                    // On rembourse la production pour retenter au tour suivant
                    city->production_stored += cost;
                    return; // On sort sans réinitialiser le projet
                }
            
                Unit *new_unit = malloc(sizeof(Unit));
                if (new_unit) {
                    if (city->current_project == PROJECT_WARRIOR) {
                        new_unit->type = UNIT_WARRIOR;
                    } else if (city->current_project == PROJECT_ARCHER) {
                        new_unit->type = UNIT_ARCHER;
                    } else if (city->current_project == PROJECT_CAVALIER) {
                        new_unit->type = UNIT_CAVALIER;
                    } else {
                        new_unit->type = UNIT_SETTLER;
                    }

                    if (new_unit->type == UNIT_WARRIOR) {
                        new_unit->hp_max = 15;
                        new_unit->attack = 3;
                        new_unit->defense = 2;
                        new_unit->pm_max = 3;
                    } else if (new_unit->type == UNIT_ARCHER) {
                        new_unit->hp_max = 12;
                        new_unit->attack = 4;
                        new_unit->defense = 1;
                        new_unit->pm_max = 2;
                    } else if (new_unit->type == UNIT_CAVALIER) {
                        new_unit->hp_max = 15;
                        new_unit->attack = 3;
                        new_unit->defense = 3;
                        new_unit->pm_max = 4;
                    } else {
                        new_unit->hp_max = 10;
                        new_unit->attack = 0;
                        new_unit->defense = 1;
                        new_unit->pm_max = 2;
                    }

                    new_unit->hp = new_unit->hp_max;
                    new_unit->pm = new_unit->pm_max;
                    
                    new_unit->x = city->x;
                    new_unit->y = city->y;
                    new_unit->owner = city->owner;


                    map->tiles[city->y][city->x].unit = new_unit;

                    player_units[*player_unit_count] = new_unit;
                    (*player_unit_count)++;

                    printw("Une nouvelle unite a ete formee !\n");
                }
                break;
            }
            default: break;
        }

        city->current_project = PROJECT_NONE;
    }
}


void control_city_project(City *city) {
    if (!city) return;

    const GameConfig *config = game_config_get_active();
    float growth_multiplier = config ? config->growth_threshold_multiplier : 1.0f;

    // Calcul du seuil de croissance pour l'affichage
    int seuil_croissance = 20 + (10 * city->population);
    if (city->has_granary) {
        seuil_croissance = (seuil_croissance * 2) / 3;
    }
    seuil_croissance = (int)(seuil_croissance * growth_multiplier + 0.5f);
    if (seuil_croissance < 1) seuil_croissance = 1;

    // Nettoyage de l'écran pour afficher le menu proprement
    clear();
    
    // Position de départ pour décoller un peu le menu des bords
    int start_y = 2;
    int start_x = 4;
    int line = start_y;

    // --- EN-TÊTE ---
    move(line++, start_x);
    attron(A_BOLD | A_UNDERLINE);
    printw("GESTION DE LA VILLE - Position [%d, %d]", city->x, city->y);
    attroff(A_BOLD | A_UNDERLINE);
    line++; // Saut de ligne

    // --- STATISTIQUES ---
    move(line++, start_x);
    printw("Population : %d    |    PV : %d", city->population, city->hp);
    
    move(line++, start_x);
    if (city->food_stored < 0) {
        attron(COLOR_PAIR(20) | A_BOLD); // Pair 20 = Rouge
        printw("[!] ALERTE FAMINE : Deficit de %d Nourriture ! La population baisse.", city->food_stored);
        attroff(COLOR_PAIR(20) | A_BOLD);
    } else {
        printw("Nourriture : %d / %d avant croissance", city->food_stored, seuil_croissance);
    }
    
    move(line++, start_x);
    printw("Production stockee : %d", city->production_stored);
    line++; 

    // --- INFRASTRUCTURES ---
    move(line++, start_x);
    attron(A_BOLD); printw("-- INFRASTRUCTURES --"); attroff(A_BOLD);
    
    move(line++, start_x);
    printw("Batiments construits : ");
    int has_buildings = 0;
    if (city->has_granary)  { printw("Grenier  "); has_buildings = 1; }
    if (city->has_workshop) { printw("Atelier  "); has_buildings = 1; }
    if (city->has_library)  { printw("Bibliotheque  "); has_buildings = 1; }
    if (city->has_market)   { printw("Marche  "); has_buildings = 1; }
    if (city->has_barrack)  { printw("Caserne  "); has_buildings = 1; }
    if (city->has_wall)     { printw("Muraille  "); has_buildings = 1; }
    if (!has_buildings) printw("Aucun");
    line++;

    // --- PROJET ACTUEL ---
    move(line++, start_x);
    attron(A_BOLD); printw("-- PROJET ACTUEL --"); attroff(A_BOLD);
    
    move(line++, start_x);
    switch(city->current_project) {
        case PROJECT_NONE: printw("Aucun projet en cours."); break;
        case PROJECT_SETTLER: printw("Production : Colon"); break;
        case PROJECT_WARRIOR: printw("Production : Guerrier"); break;
        case PROJECT_ARCHER: printw("Production : Archer"); break;
        case PROJECT_CAVALIER: printw("Production : Cavalier"); break;
        case PROJECT_GRANARY: printw("Construction : Grenier"); break;
        case PROJECT_WORKSHOP: printw("Construction : Atelier"); break;
        case PROJECT_LIBRARY: printw("Construction : Bibliotheque"); break;
        case PROJECT_MARKET: printw("Construction : Marche"); break;
        case PROJECT_BARRACKS: printw("Construction : Caserne"); break;
        case PROJECT_WALL: printw("Construction : Muraille"); break;
        default: printw("Inconnu."); break;
    }
    line++;

    // --- CHOIX DE PRODUCTION ---
    move(line++, start_x);
    attron(A_BOLD); printw("-- NOUVEAU PROJET --"); attroff(A_BOLD);
    
    move(line++, start_x); printw("Que voulez-vous produire ?");
    move(line++, start_x); printw("[0] Rien (Garder le projet actuel ou Annuler)");
    move(line++, start_x); printw("[1] Colon (50 Prod)       - Fonde une nouvelle ville");
    
    move(line++, start_x);
    if (city->has_barrack) {
        printw("[2] Guerrier (40 Prod)    - Unite militaire (Force: 3, PM: 3)");
    } else {
        attron(A_DIM); printw("[2] [BLOQUE] Guerrier     - Necessite une Caserne"); attroff(A_DIM);
    }

    if (!city->has_granary)  { move(line++, start_x); printw("[3] Grenier (30 Prod)     - Bonus : +3 Nourriture/tour"); }
    if (!city->has_workshop) { move(line++, start_x); printw("[4] Atelier (40 Prod)     - Bonus : +3 Production/tour"); }
    if (!city->has_library)  { move(line++, start_x); printw("[5] Bibliotheque (50 Prod)- Bonus : +4 Science/tour"); }
    if (!city->has_market)   { move(line++, start_x); printw("[6] Marche (40 Prod)      - Bonus : +3 Or/tour"); }
    if (!city->has_barrack)  { move(line++, start_x); printw("[7] Caserne (60 Prod)     - Bonus : Permet de recruter des Guerriers"); }
    if (!city->has_wall)     { move(line++, start_x); printw("[8] Muraille (80 Prod)    - Bonus : Double les PV max de la ville"); }

    move(line++, start_x);
    if (city->has_barrack && city->owner && city->owner->unlocked_techs[TECH_CHASSE]) {
        printw("[9] Archer (55 Prod)      - Unite avancee (Force: 4, PM: 2)");
    } else {
        attron(A_DIM); printw("[9] [BLOQUE] Archer       - Necessite Caserne + Chasse"); attroff(A_DIM);
    }

    line++;
    move(line++, start_x);
    printw("Votre choix : ");
    refresh();
    
    // Récupération du choix
    int ch = getch();
    int choix = ch - '0';

    move(line++, start_x);

    if (choix < 0 || choix > 9) {
        printw("Entree invalide ou annulee.");
    } else {
        switch (choix) {
            case 0: 
                printw("Action annulee. Le projet actuel est maintenu.");
                break;
            case 1: 
                city->current_project = PROJECT_SETTLER; city->production_stored = 0; 
                printw("Projet defini : Colon"); 
                break;
            case 2: 
                if (city->has_barrack) {
                    city->current_project = PROJECT_WARRIOR; city->production_stored = 0;
                    printw("Projet defini : Guerrier");
                } else {
                    printw("Impossible : Vous n'avez pas de caserne !");
                }
                break;
            case 3: city->current_project = PROJECT_GRANARY; city->production_stored = 0; printw("Projet defini : Grenier"); break;
            case 4: city->current_project = PROJECT_WORKSHOP; city->production_stored = 0; printw("Projet defini : Atelier"); break;
            case 5: city->current_project = PROJECT_LIBRARY; city->production_stored = 0; printw("Projet defini : Bibliotheque"); break;
            case 6: city->current_project = PROJECT_MARKET; city->production_stored = 0; printw("Projet defini : Marche"); break;
            case 7: city->current_project = PROJECT_BARRACKS; city->production_stored = 0; printw("Projet defini : Caserne"); break;
            case 8: city->current_project = PROJECT_WALL; city->production_stored = 0; printw("Projet defini : Muraille"); break;
            case 9:
                if (city->has_barrack && city->owner && city->owner->unlocked_techs[TECH_CHASSE]) {
                    city->current_project = PROJECT_ARCHER; city->production_stored = 0;
                    printw("Projet defini : Archer");
                } else {
                    printw("Impossible : Archer requis Caserne + Technologie Chasse !");
                }
                break;
        }
    }

    line++;
    move(line++, start_x);
    attron(COLOR_PAIR(3) | A_BOLD); // Couleur jaune pour mettre en évidence la sortie
    printw("[Appuyez sur une touche pour retourner au jeu...]");
    attroff(COLOR_PAIR(3) | A_BOLD);
    refresh();
    getch();
}