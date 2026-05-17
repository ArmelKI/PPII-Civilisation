#include "../../include/core/map.h"
#include "../../include/core/types.h"
#include "../../include/core/game.h"
#include "../../include/core/ressources.h"
#include "../../include/core/special_ressources.h"

#define STB_PERLIN_IMPLEMENTATION
#include "../../include/core/stb_perlin.h"

#include <stdio.h>
#include <stdlib.h>
#include <ncurses.h>

Map* create_map_noise(int width, int height, int seed) {
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
            tuiles[h][w].camp = NULL;
            /* par défaut pas de merveille ni de ressource spéciale. */
            tuiles[h][w].wonder = WONDER_NONE;
            tuiles[h][w].special_ressources = SPRES_NONE;
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

    new_map->start_x = start_x;
    new_map->start_y = start_y;

    if (!found) {
        tuiles[start_y][start_x].type = TERRAIN_PLAINE;
    }

    srand((unsigned int)seed);
    for (int y = 0; y < height; y++) {
        for (int x = 0; x < width; x++) {
            if (tuiles[y][x].type == TERRAIN_EAU) continue;
            if (x == start_x && y == start_y) continue;
            if ((rand() % 100) >= 15) continue;

            SpecialResourceType sr = sres_random_for_terrain(tuiles[y][x].type);
            if (sr != SPRES_NONE) {
                tuiles[y][x].special_ressources = (int)sr;
            }
        }
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

    start_city->owner = NULL; // On assignera le joueur plus tard

    tuiles[start_y][start_x].city = start_city;

    // Allocation du guerrier au start
    Unit*start_warrior = malloc(sizeof(Unit));
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

Map* create_map(int width, int height,int seed) {

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
            tuiles[h][w].camp = NULL;
            tuiles[h][w].wonder = WONDER_NONE;
            tuiles[h][w].special_ressources = SPRES_NONE; 
        }
    }

    // Choix de la position de la ville de départ
    // Arbitrairement au milieu
    int start_x = width / 2;
    int start_y = height / 2;

    tuiles[start_y][start_x].type = TERRAIN_PLAINE;

    new_map->start_x = start_x;
    new_map->start_y = start_y;
    new_map->height = height;
    new_map->width = width;
    new_map->tiles = tuiles;
    return new_map;
}

void display_map_color(Map* map, const Player *player, int sel_x, int sel_y) {
    if (map == NULL) return;

    for (int h = 0; h < map->height; h++) {
        for (int w = 0; w < map->width; w++) {
            Tile *tile = &map->tiles[h][w];

             // 1. Zone INCONNUE (Brouillard de guerre) -> Noir total
            if (tile->visibility == FOG_HIDDEN) {
                printw("  "); // Deux espaces pour masquer totalement la case
                continue;
            }
            // 2. Zone Visible -> On affiche les unités, villes et camps si présents
            if (tile->visibility == FOG_VISIBLE) {
            
                if (map->tiles[h][w].unit != NULL) {

                    if (w == sel_x && h == sel_y) attron(A_REVERSE);

                    switch (map->tiles[h][w].unit->type) {
                        case UNIT_BARBARIAN:
                            attron(COLOR_PAIR(20)); printw("B "); attroff(COLOR_PAIR(20)); // B pour les Barbares ennemis
                            break;
                        case UNIT_WARRIOR:
                            attron(COLOR_PAIR(10)); printw("W "); attroff(COLOR_PAIR(10)); // W pour le Guerrier du Joueur
                            break;
                        case UNIT_ARCHER:
                            attron(COLOR_PAIR(11)); printw("A "); attroff(COLOR_PAIR(11)); // A pour l'Archer
                            break;
                        case UNIT_CAVALIER:
                            attron(COLOR_PAIR(12)); printw("V "); attroff(COLOR_PAIR(12)); // V pour le Cavalier en vert
                            break;
                        case UNIT_SETTLER:
                            attron(COLOR_PAIR(13)); printw("S "); attroff(COLOR_PAIR(13)); // S Jaune brillant pour le Colon
                            break;
                        default:
                            attron(COLOR_PAIR(14)); printw("U "); attroff(COLOR_PAIR(14)); // U Violet (Unité générique) (au cas où)
                            break;
                        // A MODIFIER SUREMENT APRES SI ON RAJOUTE LES ARCHERS, ETC
                    }
                    if (w == sel_x && h == sel_y) attroff(A_REVERSE);
                    continue; // Si il y a une unité on affiche que ça et on passe à la case suivante
                }
                else if (map->tiles[h][w].city != NULL) {

                    if (w == sel_x && h == sel_y) attron(A_REVERSE);

                    if (map->tiles[h][w].city->has_wall) {
                        attron(COLOR_PAIR(31)); printw("C "); attroff(COLOR_PAIR(31)); // Blanc Brillant sur fond noir si muraille
                    } else {
                        attron(COLOR_PAIR(32)); printw("C "); attroff(COLOR_PAIR(32)); // Blanc simple avec fond gris clair
                    }
                    if (w == sel_x && h == sel_y) attroff(A_REVERSE);
                    continue; // Si il y a une ville on affiche que ça et on passe à la case suivante

                } else if (map->tiles[h][w].camp != NULL) {
                    if (map->tiles[h][w].camp->destroyed == 0) {
                        attron(COLOR_PAIR(21)); printw("▲ "); attroff(COLOR_PAIR(21)); // ▲ Rouge vif pour un camp actif
                    } else {
                        attron(COLOR_PAIR(22)); printw("△ "); attroff(COLOR_PAIR(22)); // △ Gris foncé pour un camp détruit
                    }
                    continue; // Si il y a un camp on affiche que ça et on passe à la case suivante
                }

                /* Merveille visible */
                else if (tile->wonder != WONDER_NONE) {
                    attron(COLOR_PAIR(50) | A_BOLD); printw("M "); attroff(COLOR_PAIR(50) | A_BOLD);
                    continue;
                }

                /* Ressource spéciale visible uniquement si la techno requise
                 * est débloquée (ou si player == NULL, mode debug). */
                else if (tile->special_ressources != SPRES_NONE) {
                    SpecialResourceType sr = (SpecialResourceType)tile->special_ressources;
                    if (!player || sres_visible(player, sr)) {
                        const SpecialResourceDef *def = sres_get(sr);
                        if (def) {
                            int pair = 51 + (int)sr; /* 51..55 selon le type */
                            attron(COLOR_PAIR(pair) | A_BOLD);
                            printw("%c ", def->ascii_char);
                            attroff(COLOR_PAIR(pair) | A_BOLD);
                            continue;
                        }
                    }
                    /* Sinon : on retombe sur l'affichage du terrain normal */
                }
                
            }

            if (tile->visibility == FOG_EXPLORED) {
                // Si juste "Explorée", on affiche uniquement le caractère du terrain en gris foncé
                char t_char = '?';
                int color_pair = 40; // Couleur par défaut pour les terrains explorés
                switch (tile->type) {
                    case TERRAIN_EAU:      t_char = '~'; break;
                    case TERRAIN_PLAINE:   t_char = '.'; break;
                    case TERRAIN_MONTAGNE: t_char = '^'; break;
                    case TERRAIN_FORET:    t_char = 'f'; break;
                    case TERRAIN_DESERT:   t_char = '_'; break;
                    case TERRAIN_TOUNDRA:  t_char = '*'; break;
                    default:               t_char = '?'; break;
                }
                attron(COLOR_PAIR(color_pair) | A_DIM); 

                printw("%c ", t_char); 
                attroff(COLOR_PAIR(color_pair) | A_DIM);
            }
            
            else {
                // Zone visible et vide : On affiche le terrain avec ses vraies couleurs
                City* city_case = get_tile_owner(w,h,map);
                bool is_exploited = (city_case !=  NULL);
                int exploited = is_exploited ? 50 : 0; // Si la tuile est exploitée on change le fond de couleur de la case
                if (is_exploited && city_case->has_wall) exploited = 60;
                switch (map->tiles[h][w].type) {
                    case TERRAIN_EAU:      
                        attron(COLOR_PAIR(1 + exploited)); printw("~ "); attroff(COLOR_PAIR(1 + exploited)); break; // Bleu
                    case TERRAIN_PLAINE:   
                        attron(COLOR_PAIR(2 + exploited)); printw(". "); attroff(COLOR_PAIR(2 + exploited)); break; // Vert
                    case TERRAIN_MONTAGNE: 
                        attron(COLOR_PAIR(4 + exploited)); printw("^ "); attroff(COLOR_PAIR(4 + exploited)); break; // Jaune/Marron
                    case TERRAIN_FORET:    
                        attron(COLOR_PAIR(3 + exploited)); printw("f "); attroff(COLOR_PAIR(3 + exploited)); break; // Vert foncé
                    case TERRAIN_DESERT:   
                        attron(COLOR_PAIR(5 + exploited)); printw("_ "); attroff(COLOR_PAIR(5 + exploited)); break; // Jaune brillant
                    case TERRAIN_TOUNDRA:
                        attron(COLOR_PAIR(6 + exploited)); printw("* "); attroff(COLOR_PAIR(6 + exploited)); break; // Blanc
                    default:               
                        printw("? "); break;
                }
            }
        }
        printw("\n");
    }
}

// Fonction utilitaire pour révéler une zone (Distance de Tchebychev)
void reveal_area(GameState *game, int center_x, int center_y, int radius) {
    for (int dy = -radius; dy <= radius; dy++) {
        for (int dx = -radius; dx <= radius; dx++) {
            int nx = center_x + dx;
            int ny = center_y + dy;
            //Vérifier qu'on ne sort pas des limites de la carte
            if (nx >= 0 && nx < game->map.width && ny >= 0 && ny < game->map.height) {
                game->map.tiles[ny][nx].visibility = FOG_VISIBLE;
            }
        }
    }
}

// Fonction utilitaire à appeler à chaque tour/action pour mettre à jour la carte
void update_fog_of_war(GameState *game, int is_fow_enabled) {
    
    // Si l'option Brouillard est désactivée, on force toute la carte en VISIBLE
    if (!is_fow_enabled) {
        for (int y = 0; y < game->map.height; y++) {
            for (int x = 0; x < game->map.width; x++) {
                game->map.tiles[y][x].visibility = FOG_VISIBLE;
            }
        }
        return;
    }

    // 1. Assombrir ce qui n'est plus dans le champ de vision
    // Tout ce qui est visible devient simplement EXPLORED
    for (int y=0; y<game->map.height; y++) {
        for (int x=0; x<game->map.width; x++) {
            if (game->map.tiles[y][x].visibility == FOG_VISIBLE) {
                game->map.tiles[y][x].visibility = FOG_EXPLORED;
            }
        }
    }

    // 2. Éclairer autour des villes (Rayon 3 d'après le Readme)
    for (int i=0; i<game->city_count; i++) {
        if (game->cities[i] != NULL) {
            reveal_area(game, game->cities[i]->x, game->cities[i]->y, 3);
        }
    }

    // 3. Éclairer autour des unités (Rayon 2 d'après le Readme)
    for (int i=0; i<game->player_unit_count; i++) {
        Unit *u = game->player_units[i];
        if (u != NULL && u->hp > 0) {
            reveal_area(game, u->x, u->y, 2);
        }
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
        attron(COLOR_PAIR(5) | A_BOLD);
        printw("[Erreur] Position (%d, %d) hors limites.\n", x, y);
        attroff(COLOR_PAIR(5) | A_BOLD);
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
