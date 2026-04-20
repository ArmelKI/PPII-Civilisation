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

            printf("Une unité de type %d a été licenciée pour des raisons économiques en [%d,%d]..\n",martyr_unit -> type, martyr_unit -> x, martyr_unit-> y );

            map->tiles[martyr_unit->y][martyr_unit->x].unit = NULL;

        } else {

            printf("Plus aucune unité à dissoudre, on constate une GRÈVE OUVRIÈRE, nos capacités ouvrières son 2 fois moins importantes.\n");

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
