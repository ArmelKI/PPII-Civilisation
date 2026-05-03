#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "../../include/core/game.h"
#include "../../include/core/physics.h"
#include "../../include/core/tech.h"
#include "../../include/core/ressources.h"
#include "../../include/core/config.h"

void init_game(GameState *game, int width, int height, int seed) {
    memset(game, 0, sizeof(GameState));
    game->seed = seed;
    game->turn = 1;
    game->game_over = 0;

    // Initialisation Joueur
    game->player.gold = 10;
    game->player.science = 0;

    // Initialisation de l'arbre technologique 
    game->player.current_research = TECH_NONE;
    for (int i = 0; i < MAX_TECHNOLOGIES; i++) {
        game->player.unlocked_techs[i] = 0;
    }

    // Génération de la map
    Map *map_ptr = create_map_noise(width, height, seed);
    game->map = *map_ptr;
    free(map_ptr);

    // Initialisation de la Ville de départ (algo dans le create_map_noise)
    game -> cities[0] = malloc(sizeof(City));
    game ->city_count = 1;
    if (game -> cities[0] != NULL) {
        game->cities[0]->x = game->map.start_x;
        game->cities[0]->y = game->map.start_y;
        game->cities[0]->hp = 20;
        game->cities[0]->population = 2;
        game->cities[0]->owner = &game->player;
        game->map.tiles[game->cities[0]->y][game->cities[0]->x].city = game->cities[0];
}
    // Initialisation Guerrier de départ
    Unit *warrior = malloc(sizeof(Unit));
    if (warrior && game->cities[0] != NULL) {
        warrior->type = UNIT_WARRIOR;
        warrior->hp = 15;
        warrior->hp_max = 15;
        warrior->attack = 3;
        warrior->defense = 2;
        warrior->pm = 3;
        warrior->pm_max = 3;
        warrior->x = game->cities[0]->x;
        warrior->y = game->cities[0]->y;
        warrior->owner = &game->player;
        
        game->player_units[0] = warrior;
        game->player_unit_count = 1;
        game->map.tiles[warrior->y][warrior->x].unit = warrior;
    }
}

void restore_pm(Unit **units, int count) {
    // Début de tour : toutes les unités vivantes récupèrent leurs PM max.
    for (int i = 0; i < count; ++i) {
        if (!units[i] || units[i]->hp <= 0) {
            continue;
        }
        units[i]->pm = units[i]->pm_max;
    }
}

void spawn_random_barbarian(GameState *game) {
    // Plafond global de barbares (version test, simple à ajuster ensuite).
    if (game->barbarian_count >= MAX_BARBARIANS) {
        return;
    }

    // Probabilité de spawn à chaque tour.
    int spawn_chance = game_config_barbarian_spawn_chance();
    if ((rand() % 100) >= spawn_chance) {
        return;
    }

    int x = rand() % game->map.width;
    int y = rand() % game->map.height;

    if (game->map.tiles[y][x].type == TERRAIN_EAU) {
        return;
    }

    if (game->map.tiles[y][x].unit || game->map.tiles[y][x].city) {
        return;
    }

    Unit *barb = (Unit *)malloc(sizeof(Unit));
    if (!barb) {
        return;
    }

    barb->type = UNIT_BARBARIAN;
    barb->defense = 2;
    barb->x = x;
    barb->y = y;
    barb->owner = NULL;
    game_config_apply_barbarian_stats(barb);

    game->barbarians[game->barbarian_count++] = barb;
    game->map.tiles[y][x].unit = barb;
}

void move_barbarians_random(GameState *game) {
    // Déplacement aléatoire d'une case : utile pour valider la phase Barbares.
    for (int i = 0; i < game->barbarian_count; ++i) {
        Unit *barb = game->barbarians[i];
        if (!barb || barb->hp <= 0) {
            continue;
        }

        int dx = (rand() % 3) - 1;
        int dy = (rand() % 3) - 1;
        if (dx == 0 && dy == 0) {
            continue;
        }

        int nx = barb->x + dx;
        int ny = barb->y + dy;

        if (nx < 0 || nx >= game->map.width || ny < 0 || ny >= game->map.height) {
            continue;
        }

        if (game->map.tiles[ny][nx].type == TERRAIN_EAU) {
            continue;
        }

        if (game->map.tiles[ny][nx].unit || game->map.tiles[ny][nx].city) {
            continue;
        }

        game->map.tiles[barb->y][barb->x].unit = NULL;
        barb->x = nx;
        barb->y = ny;
        barb->pm -= 1;
        game->map.tiles[barb->y][barb->x].unit = barb;
    }
}

void next_turn(GameState *game) {
    // 1) Restauration PM
    restore_pm(game->player_units, game->player_unit_count);
    restore_pm(game->barbarians, game->barbarian_count);

    // 2) Revenus des villes (Boucle sur toutes les villes possédées)
    for (int i = 0; i < game->city_count; i++) {
        if (game->cities[i] != NULL) {
            process_city_resources(game->cities[i], &game->map);
            process_city_growth(game->cities[i], &game->map);
            process_city_production(game->cities[i], &game->map, game->player_units, &game->player_unit_count);
        }
    }
    
    // Le joueur paie l'entretien global (une seule fois par tour, avec le &)
    process_player_gold(&game->player, &game->map);

    // 3) Phase Barbares (spawn + déplacement)
    spawn_random_barbarian(game);
    move_barbarians_random(game);

    // 4) mise à jour de la recherche des technologies
    update_research(&game->player);

    game->turn++;
}

void cleanup_game(GameState *game) {
    for (int i = 0; i < game->city_count; ++i) {
        if (game->cities[i]) {
            free(game->cities[i]);
        }
    }
    for (int i = 0; i < game->player_unit_count; ++i) {
        if (game->player_units[i]) {
            free(game->player_units[i]);
        }
    }

    for (int i = 0; i < game->barbarian_count; ++i) {
        free(game->barbarians[i]);
    }

    if (game->map.tiles) {
        for (int i = 0; i < game->map.height; i++) {
            free(game->map.tiles[i]);
        }
        free(game->map.tiles);
    }

    if (game->tile_buffer) {
        free(game->tile_buffer);
    }
}