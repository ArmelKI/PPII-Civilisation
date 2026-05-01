#include "../../include/core/map.h"

#define STB_PERLIN_IMPLEMENTATION
#include "../../include/core/stb_perlin.h"

#include <stdio.h>
#include <stdlib.h>

Map* create_map_noise(int width, int height,Player* player, int seed) {
    // paramètres modifiables en fonction de nos préférences
    // FAIRE DANS LE README UN TRUC VRAIMENT BIEN DETAILLÉ

    // Allocation de la carte
    Map* new_map = malloc(sizeof(Map));
    if (new_map == NULL) return NULL;

    Tile** tuiles = malloc(height * sizeof(Tile*));
    if (tuiles == NULL) return NULL;

    for (int i = 0; i < height; i++) {
        tuiles[i] = malloc(width*sizeof(Tile));
        if (tuiles[i] == NULL) return NULL;
    }

    // génération avec le bruit de perlin

    // FONCTIONNEMENT : la fonction renvoie un nombre entre -1 et 1 donc ensuite on 
    // utilise des "paliers" pour chaque type de terrain
    for (int h = 0; h < height; h++) {
        for (int w = 0; w < width; w++) {
            float scale = 0.1f;

            float noise = stb_perlin_noise3_seed(w*scale, h*scale, (float)seed * 0.1f, 0, 0, 0, (unsigned char) seed);

            // On ramène la valeur entre 0.0 et 1.0 pour que ce soit plus simple
            float val = (noise + 1.0f) / 2.0f;

            if (val < 0.40f) {
                tuiles[h][w].type = TERRAIN_EAU;
            }
            else if (val < 0.43f) {
                tuiles[h][w].type = TERRAIN_DESERT;
            }
            else if (val < 0.65f) {
                tuiles[h][w].type = TERRAIN_PLAINE;
            }
            else if (val < 0.74f) {
                tuiles[h][w].type = TERRAIN_FORET;
            }
            else if (val < 0.82f) {
                tuiles[h][w].type = TERRAIN_MONTAGNE;
            }
            else {
                tuiles[h][w].type = TERRAIN_TOUNDRA;
            }

            tuiles[h][w].city = NULL;
            tuiles[h][w].unit = NULL;
        }
    }

    // Choix de la position de la ville de départ
    // Recherche depuis le centre en augmentant le rayon de recherche à chaque tour
    // pour trouver la plaine la plus au centre
    int start_x = width / 2;
    int start_y = height / 2;
    int found = 0;

    for (int rayon = 0; (rayon < width && !found); rayon++) {
        for (int dy = -rayon; dy <= rayon && !found; dy++) {
            for (int dx = -rayon; dx <= rayon && !found; dx++) {
                int cur_y = start_y + dy;
                int cur_x = start_x + dx;

                // verif des limites de la map
                if (cur_x >= 0 && cur_x < width && cur_y >= 0 && cur_y < height) {
                    // On cherche un TERRAIN_PLAINE
                    if (tuiles[cur_y][cur_x].type == TERRAIN_PLAINE) {
                        start_x = cur_x;
                        start_y = cur_y;
                        found = 1;
                    }
                }
            }
        }
    }

    if (!found) {
        tuiles[start_y][start_x].type = TERRAIN_PLAINE;
    }

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

Map* create_map(int width, int height,Player* player,int seed) {

	srand(seed); // On utilise la seed générée ou donnée par l'utilisateur 
                 //pour générer la prochaine suite de nombres

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
                    case TERRAIN_TOUNDRA:  printf("* "); break;
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
                switch (map->tiles[h][w].unit->type) {
                    case UNIT_BARBARIAN:
                        printf("\033[1;31mB \033[0m"); // B pour les Barbares ennemis
                        break;
                    case UNIT_WARRIOR:
                        printf("\033[1;36mW \033[0m"); // W pour le Guerrier du Joueur
                        break;
                    case UNIT_SETTLER:
                        printf("\033[1;33mS \033[0m"); // S Jaune brillant pour le Colon
                        break;
                    default:
                        printf("\033[1;35mU \033[0m"); // U Violet (Unité générique) (au cas où)
                        break;
                    // A MODIFIER SUREMENT APRES SI ON RAJOUTE LES ARCHERS, ETC
                }
            }
            else if (map->tiles[h][w].city != NULL) {
                if (map->tiles[h][w].city->has_wall) {
                    printf("\033[1;37;40mC \033[0m"); // Blanc Brillant sur fond noir si muraille
                } else {
                    printf("\033[0;37mC \033[0m");    // Blanc simple
                }

            } else if (map->tiles[h][w].camp != NULL) {
                if (map->tiles[h][w].camp->destroyed == 0) {
                    printf("\033[1;31m▲ \033[0m"); // ▲ Rouge vif pour un camp actif
                } else {
                    printf("\033[1;30m△ \033[0m"); // △ Gris foncé pour un camp détruit
                }
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
                    case TERRAIN_TOUNDRA:
                        printf("* "); break; // Blanc
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
