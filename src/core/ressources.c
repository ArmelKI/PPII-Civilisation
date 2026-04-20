#include <stdio.h>
#include <stdlib.h>
#include "core/types.h"

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
    city->production_stored += total_yield.production;
    
    city->owner->gold += total_yield.gold;
    city->owner->science += total_yield.science;

}


//fonction qui gère la croissance et la famine
void process_city_growth(City *city,Map *map){

    if (!city || !map) return;

    int growth_seuil= 20 + (10* city -> population);

    if(city -> has_granary){
        growth_seuil = (growth_seuil*2)/3;
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

    if( city -> food_stored < 0){

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

                if(u -> type == UNIT_WARRIOR){
                    total_maintenance += 1;
                }

            }
        }
    }
    player -> gold -= total_maintenance;

    if(player -> gold < 0){
        printf("ALERTE BANQUEROUTE ! Découvert de %d Or.\n", player->gold);

        if (unit_count > 0) {
            int random_index = rand() % unit_count;
            Unit *martyr_unit = player_units[random_index];

            printf("Une unité de type %d a été licenciée pour des raisons économiques en [%d,%d]...\n",martyr_unit -> type, martyr_unit -> x, martyr_unit-> y );

            map->tiles[martyr_unit->y][martyr_unit->x].unit = NULL;

        } else {

            printf("Plus aucune unité à dissoudre, on constate une GRÈVE OUVRIÈRE, nos capacités ouvrières son deux fois moins importantes...\n");

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
int found_city(Unit *settler, Map *map, int *city_id_counter){
    if(!settler||!map|| !city_id_counter) return 0;
    if(settler->type != UNIT_SETTLER) {
        printf("Seul un Colon peut fonder une ville !\n");
        return 0;
    }

    int sx = settler -> x;
    int sy = settler -> y;
    Tile *target_tile = &(map->tiles[sy][sx]);

    if(target_tile->type == TERRAIN_EAU) {
        printf("Impossible de fonder une ville sur l'eau.\n");
        return 0;
    }
    if(target_tile->city != NULL) {
        printf("Une ville existe déjà.\n");
        return 0;
    }

    for(int dy = -3; dy <= 3; dy++) {
        for (int dx = -3; dx <= 3; dx++) {
            int cx = sx + dx;
            int cy = sy + dy;
            
            if(cx >= 0 && cx < map->width && cy >= 0 && cy < map->height) {
                if (map->tiles[cy][cx].city != NULL) {
                    printf("Il exite déjà une ville à moins de 4 cases.\n");
                    return 0; 
                }
            }
        }
    }


    //aucunde condition éliminatoire : fondation de la ville

    City *new_city = malloc(sizeof(City));
    if (!new_city) {
        printf("Erreur mémoire lors de la création de la ville.\n");
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

    target_tile->unit = NULL;
    free(settler);

    printf("Une nouvelle ville vient d'être fondée en [%d, %d] !\n", sx, sy);
    return 1;
}


//gestion de la production de bâtiments et d'unités
void process_city_production(City *city, Map *map) {
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
        default: return;
    }

    if (city->production_stored >= cost) {

        city->production_stored -= cost; 

        switch (city->current_project) {
            
            case PROJECT_GRANARY:  city->has_granary = 1;  printf("Grenier construit !\n"); break;
            case PROJECT_WORKSHOP: city->has_workshop = 1; printf("Atelier construit !\n"); break;
            case PROJECT_LIBRARY:  city->has_library = 1;  printf("Bibliothèque construite !\n"); break;
            case PROJECT_MARKET:   city->has_market = 1;   printf("Marché construit !\n"); break;
            case PROJECT_BARRACKS: city->has_barrack = 1; printf("Caserne construite !\n"); break;
            case PROJECT_WALL: {
                city->has_wall = 1;
                city->hp *= 2; 
                printf("Muraille construite !\n");
                break;
            }


            case PROJECT_WARRIOR:
            case PROJECT_SETTLER: {

                Unit *new_unit = malloc(sizeof(Unit));
                if (new_unit) {
                    new_unit->type = (city->current_project == PROJECT_WARRIOR) ? UNIT_WARRIOR : UNIT_SETTLER;
                    
                    new_unit->hp_max = (new_unit->type == UNIT_WARRIOR) ? 15 : 10;
                    new_unit->hp = new_unit->hp_max;
                    new_unit->attack = (new_unit->type == UNIT_WARRIOR) ? 3 : 0;
                    new_unit->defense = (new_unit->type == UNIT_WARRIOR) ? 2 : 1;
                    new_unit->pm_max = (new_unit->type == UNIT_WARRIOR) ? 3 : 2;
                    new_unit->pm = new_unit->pm_max;
                    
                    new_unit->x = city->x;
                    new_unit->y = city->y;
                    new_unit->owner = city->owner;


                    map->tiles[city->y][city->x].unit = new_unit;
                    printf("Une nouvelle unite a été formee !\n");
                }
                break;
            }
            default: break;
        }

        city->current_project = PROJECT_NONE;
    }
}