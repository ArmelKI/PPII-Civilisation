#include "../../include/core/map.h"
#include <stdio.h>
#include <stdlib.h>

Map* create_map(int width, int height,Player* player) {

    // Allocation de la carte
    Map* new_map = malloc(sizeof(Map));
    if (new_map == NULL) return NULL;

    Tile** tuiles = malloc(height * sizeof(Tile*));
    if (tuiles == NULL) return NULL;

    for (int i = 0; i < height; i++) {
        tuiles[i] = malloc(width*sizeof(Tile));
        if (tuiles[i] == NULL) return NULL;
    }

    // génération naïve
    for (int h = 0; h < height; h++) {
        for (int w = 0; w < width; w++) {
            tuiles[h][w].type = (TerrainType)(rand() % 5);
            tuiles[h][w].city = NULL;
            tuiles[h][w].unit = NULL;
        }
    }

    // Choix de la position de la ville de départ
    // Arbitrairement au milieu
    int start_x = width / 2;
    int start_y = height / 2;

    tuiles[start_y][start_x].type = TERRAIN_PLAINE;

    // Allocation de la ville
    City* start_city = malloc(sizeof(City));
    start_city->x = start_x;
    start_city->y = start_y;
    start_city->hp = 100;
    start_city->population = 1;
    start_city->has_wall = 0;
    start_city->food_stored = 0;
    start_city->production_stored = 0;

    start_city->owner = player;

    tuiles[start_y][start_x].city = start_city;

    // Allocation du guerrier au start
    Unit* start_warrior = malloc(sizeof(Unit));
    if (start_warrior != NULL) {
        start_warrior->type = UNIT_WARRIOR;
        start_warrior->x = start_x;
        start_warrior->y = start_y;
        start_warrior->hp = 20;
        start_warrior->hp_max = 20;
        start_warrior->attack = 3;
        start_warrior->defense = 2;
        start_warrior->pm = 2;
        start_warrior->pm_max = 2;
        start_warrior->owner = start_city->owner; // Même joueur que la ville

        tuiles[start_y][start_x].unit = start_warrior;
    }

    new_map->height = height;
    new_map->width = width;
    new_map->tiles = tuiles;
    return new_map;
}

void display_map(Map* map) {
    if (map == NULL) return;

    // On affiche en priorité les units -> ville -> terrain
    for (int h = 0; h < map->height; h++) {
        for (int w = 0; w < map->width; w++) {
            // 1. Priorité à l'unité
            if (map->tiles[h][w].unit != NULL) {
                printf("U "); // U pour Unité
            }
            else if (map->tiles[h][w].city != NULL) {
                printf("C "); // C pour Cité
            }
            else {
                switch (map->tiles[h][w].type) {
                    case TERRAIN_PLAINE:   printf(". "); break;
                    case TERRAIN_EAU:      printf("~ "); break;
                    case TERRAIN_MONTAGNE: printf("^ "); break;
                    case TERRAIN_FORET:    printf("f "); break;
                    case TERRAIN_DESERT:   printf("_ "); break;
                    default:               printf("? "); break;
                }
            }
        }
        printf("\n");
    }
}

void display_map_color(Map* map) {
    if (map == NULL) return;

    for (int h = 0; h < map->height; h++) {
        for (int w = 0; w < map->width; w++) {
            
            if (map->tiles[h][w].unit != NULL) {
                printf("\033[1;31mU \033[0m"); 
            } 
            else if (map->tiles[h][w].city != NULL) {
                printf("\033[1;37mC \033[0m"); 
            } 
            else {
                switch (map->tiles[h][w].type) {
                    case TERRAIN_EAU:      
                        printf("\033[0;34m~ \033[0m"); break; // Bleu
                    case TERRAIN_PLAINE:   
                        printf("\033[0;32m. \033[0m"); break; // Vert
                    case TERRAIN_MONTAGNE: 
                        printf("\033[0;33m^ \033[0m"); break; // Jaune/Marron
                    case TERRAIN_FORET:    
                        printf("\033[2;32mf \033[0m"); break; // Vert foncé
                    case TERRAIN_DESERT:   
                        printf("\033[0;33;1m_ \033[0m"); break; // Jaune brillant
                    default:               
                        printf("? "); break;
                }
            }
        }
        printf("\n");
    }
}

void free_map(Map* map) {
    if (map == NULL) return;

    for (int h = 0; h < map->height; h++) {
        for (int w = 0; w < map->width; w++) {

            if (map->tiles[h][w].unit != NULL) {
                free(map->tiles[h][w].unit);
            }

            if (map->tiles[h][w].city != NULL) {
                free(map->tiles[h][w].city);
            }
        }
        free(map->tiles[h]); 
    }
    
    free(map->tiles);
    free(map);
}

void set_unit_at(Map* map, int x, int y, Unit* unit) {
    if (map == NULL || unit == NULL) return;

    // Hors des limites de la map -> erreur
    if (x < 0 || x >= map->width || y < 0 || y >= map->height) {
        fprintf(stderr, "\033[1;31m[Erreur]\033[0m Position (%d, %d) hors limites.\n", x, y);
        return;
    }

    // S'il y avait une ancienne unité sur la tuile on la libère
    if (map->tiles[y][x].unit != NULL && map->tiles[y][x].unit != unit) {
        free(map->tiles[y][x].unit);
    }

    unit->x = x;
    unit->y = y;
    map->tiles[y][x].unit = unit;
}
