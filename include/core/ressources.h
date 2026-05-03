#ifndef RESSOURCES_H
#define RESSOURCES_H

#include "core/types.h"

TileYield get_terrain_yield(TerrainType type);
int get_chebyshev_distance(int x1, int y1, int x2, int y2);
City* get_tile_owner(int target_x, int target_y, Map* map);
void process_city_resources(City *city, Map *map);
void process_city_growth(City *city,Map *map);
void process_player_gold(Player *player, Map *map);
int found_city(Unit *settler, Map *map, int *city_id_counter);
void process_city_production(City *city, Map *map);
void control_city_project(City *city);

#endif
