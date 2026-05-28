#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdarg.h>
#include "../../include/core/game.h"
#include "../../include/core/barbares.h"
#include "../../include/core/physics.h"
#include "../../include/core/tech.h"
#include "../../include/core/ressources.h"
#include "../../include/core/config.h"

static int count_living_barbarians(const GameState *game)
{
    int count = 0;

    if (!game) {
        return 0;
    }

    for (int i = 0; i < game->barbarian_count; ++i) {
        if (game->barbarians[i] && game->barbarians[i]->hp > 0) {
            count++;
        }
    }

    return count;
}

void game_set_combat_log(GameState *game, const char *fmt, ...)
{
    va_list args;

    if (!game || !fmt) {
        return;
    }

    va_start(args, fmt);
    vsnprintf(game->combat_log, sizeof(game->combat_log), fmt, args);
    va_end(args);
    game->combat_log_ready = 1;
}

/*
 * Compacte le tableau `game->barbarians` en retirant les unités mortes.
 * - Libère la mémoire des barbares morts et nettoie les pointeurs sur la carte.
 * - Maintient `game->barbarian_count` cohérent après compactage.
 */
static void compact_barbarians(GameState *game)
{
    int write_index = 0;

    if (!game) {
        return;
    }

    for (int read_index = 0; read_index < game->barbarian_count; ++read_index) {
        Unit *barb = game->barbarians[read_index];
        if (!barb) {
            continue;
        }

        if (barb->hp <= 0) {
            if (barb->x >= 0 && barb->x < game->map.width && barb->y >= 0 && barb->y < game->map.height) {
                if (game->map.tiles[barb->y][barb->x].unit == barb) {
                    game->map.tiles[barb->y][barb->x].unit = NULL;
                }
            }
            free(barb);
            continue;
        }

        game->barbarians[write_index++] = barb;
    }

    for (int i = write_index; i < game->barbarian_count; ++i) {
        game->barbarians[i] = NULL;
    }

    game->barbarian_count = write_index;
}

static void compact_player_units(GameState *game)
{
    int write_index = 0;

    if (!game) {
        return;
    }

    for (int read_index = 0; read_index < game->player_unit_count; ++read_index) {
        Unit *u = game->player_units[read_index];
        if (!u) {
            continue;
        }

        if (u->hp <= 0) {
            free(u);
            continue;
        }

        game->player_units[write_index++] = u;
    }

    for (int i = write_index; i < game->player_unit_count; ++i) {
        game->player_units[i] = NULL;
    }

    game->player_unit_count = write_index;
}

void init_game(GameState *game, int width, int height, int seed, int barbarian_camp_count, int nb_turns) {
    memset(game, 0, sizeof(GameState));
    game->seed = seed;
    game->turn = 1;
    game->max_turns = (nb_turns > 0) ? nb_turns : MAX_TURNS;
    game->game_over = 0;
    game ->consecutive_turns_10_cities = 0;
    game -> consecutive_zero_production_turns = 0;

    /* EXT-2 : memset() laisse host_city_id à 0, ce qui se confondrait
     * avec une ville valide. On force -1 (= "personne"). Les autres
     * champs wonder_built/x/y sont bien à 0 grâce au memset. */
    for (int i = 0; i < NUM_WONDERS; i++) {
        game->wonder_host_city_id[i] = -1;
    }

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
        game->cities[0]->id = 0;
        game->cities[0]->current_project = PROJECT_NONE;
        game->cities[0]->has_wall = 1;
        game->cities[0]->has_granary = 0;
        game->cities[0]->has_workshop = 0;
        game->cities[0]->has_library = 0;
        game->cities[0]->has_market = 0;
        game->cities[0]->has_barrack = 0;
        game->map.tiles[game->cities[0]->y][game->cities[0]->x].city = game->cities[0];
}

    if (barbarian_camp_count <= 0) {
        barbarian_camp_count = 3;
    }

    game->barbarian_camp_count = barbarian_camp_count;
    game->barbarian_camps_destroyed = 0;
    game->barbarian_camps = barbares_init_camps(&game->map, barbarian_camp_count,
                                               game->cities[0] ? game->cities[0]->x : game->map.start_x,
                                               game->cities[0] ? game->cities[0]->y : game->map.start_y);
    if (game->barbarian_camps) {
        for (int i = 0; i < game->barbarian_camp_count; ++i) {
            BarbarianCamp *camp = &game->barbarian_camps[i];
            if (camp->x >= 0 && camp->x < game->map.width && camp->y >= 0 && camp->y < game->map.height) {
                game->map.tiles[camp->y][camp->x].camp = camp;
            }
        }
    } else {
        game->barbarian_camp_count = 0;
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

/*
 * Gère la phase de génération de barbares.
 * - Parcourt chaque camp actif et tente de faire apparaître une unité sur la case
 *   du camp si elle est libre (appelant `barbares_spawn_unit`).
 * - Enforce le plafond global : nombre maximal de barbares vivants = min(N*3, MAX_BARBARIANS).
 * - Le paramètre N correspond à `game->barbarian_camp_count`.
 */
void spawn_random_barbarian(GameState *game) {
    if (!game || !game->barbarian_camps || game->barbarian_camp_count <= 0) {
        return;
    }

    // La difficulté agit ici comme un pourcentage de réussite par camp.
    // On garde le modèle "un essai par camp" et on laisse la config moduler
    // la probabilité qu'un camp produise effectivement une unité ce tour.
    int spawn_chance = game_config_barbarian_spawn_chance();
    int max_living_barbarians = game->barbarian_camp_count * 3;
    if (max_living_barbarians > MAX_BARBARIANS) {
        max_living_barbarians = MAX_BARBARIANS;
    }

    if (count_living_barbarians(game) >= max_living_barbarians) {
        return;
    }

    for (int i = 0; i < game->barbarian_camp_count; ++i) {
        if (count_living_barbarians(game) >= max_living_barbarians) {
            break;
        }

        BarbarianCamp *camp = &game->barbarian_camps[i];
        if (!camp || camp->destroyed) {
            continue;
        }

        if ((rand() % 100) >= spawn_chance) {
            continue;
        }

        int spawned = barbares_spawn_unit(camp, &game->map, game->barbarians, &game->barbarian_count,
                                          max_living_barbarians);
        if (!spawned) {
            continue;
        }
    }
}

void move_barbarians_random(GameState *game) {
    compact_barbarians(game);

    for (int i = 0; i < game->barbarian_count; ++i) {
        Unit *barb = game->barbarians[i];
        if (!barb || barb->hp <= 0) {
            continue;
        }

        char combat_log[256];
        combat_log[0] = '\0';

        if (barbares_attack_if_adjacent(barb, &game->map, game->cities, game->city_count,
                                        game->player_units, game->player_unit_count,
                                        combat_log, sizeof(combat_log))) {
            if (combat_log[0] != '\0') {
                game_set_combat_log(game, "%s", combat_log);
            }
            continue;
        }

        barbares_move(barb, &game->map, game->cities, game->city_count,
                      game->player_units, game->player_unit_count);

        combat_log[0] = '\0';
        if (barbares_attack_if_adjacent(barb, &game->map, game->cities, game->city_count,
                                        game->player_units, game->player_unit_count,
                                        combat_log, sizeof(combat_log))) {
            if (combat_log[0] != '\0') {
                game_set_combat_log(game, "%s", combat_log);
            }
        }
    }
}

void next_turn(GameState *game) {
    // 1) Restauration PM
    compact_barbarians(game);
    restore_pm(game->player_units, game->player_unit_count);
    restore_pm(game->barbarians, game->barbarian_count);

    // 2) Revenus des villes (Boucle sur toutes les villes possédées)
    for (int i = 0; i < game->city_count; i++) {
        if (game->cities[i] != NULL) {
            process_city_resources(game->cities[i], &game->map, game);
            process_city_growth(game->cities[i], &game->map, game);
            process_city_production(game->cities[i], &game->map, game->player_units, &game->player_unit_count, game);
        }
    }
    
    // Le joueur paie l'entretien global (une seule fois par tour, avec le &)
    process_player_gold(&game->player, &game->map);
    compact_player_units(game);

    // 3) mise à jour de la recherche des technologies (avant la phase barbares)
    update_research(&game->player, game);

    // 4) Phase Barbares (spawn + déplacement)
    spawn_random_barbarian(game);
    move_barbarians_random(game);

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

    barbares_cleanup(game->barbarian_camps, game->barbarian_camp_count);

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

void check_victory_and_defeat(GameState *game) {
    int active_cities = 0;
    int total_techs = 0;
    int total_production_current_turn = 0;

    for (int i = 0; i< MAX_TECHNOLOGIES; i++) {
        if (game->player.unlocked_techs[i]) {
            total_techs++;
        }
    }

    for (int i = 0; i < game->city_count; i++) {
        if (game->cities[i] && game->cities[i]->hp > 0) {
            active_cities++;
            total_production_current_turn += game->cities[i]->production_stored;
        }
    }

    // CONDITIONS DE DEFAITE
    if (active_cities == 0) {
        game->game_over = 2;  // 1 = victoire, 2 = défaite donc Défaite plus de villes
        return;
    }

    if (game->turn > game->max_turns) {
        game->game_over = 2;    // Défaite temps écoulé
        return;
    }

    // Faillite de production 
    if (total_production_current_turn <= 0) {
        game->consecutive_zero_production_turns++;
        if (game->consecutive_zero_production_turns >= 5) {
            game->game_over = 2; // Défaite par faillite
            return;
        }
    } else {
        // Dès qu'on génère à nouveau de la production, on remet à zéro !
        game->consecutive_zero_production_turns = 0;
    }

    // CONDITIONS DE VICTOIRE
    if (total_techs >= MAX_TECHNOLOGIES) {
        game->game_over = 1;        // Victoire scientifique
        return;
    }
    if (active_cities >= 10) {
        game -> consecutive_turns_10_cities ++;
        if (game -> consecutive_turns_10_cities >= 5) {
            game->game_over = 1;        // Victoire territoriale
            return;
        }
    } else {
        game -> consecutive_turns_10_cities = 0;
    }
}

// Calcule le score mathématique selon la formule du README

int calculate_final_score(GameState *game) {
    int total_techs = 0;
    int total_pop = 0;
    int active_cities = 0;

    for (int i = 0; i< MAX_TECHNOLOGIES; i++) {
        if (game->player.unlocked_techs[i]) {
            total_techs++;
        }
    }
    for (int i = 0; i < game->city_count; i++) {
        if (game->cities[i]) {
            active_cities++;
            total_pop += game->cities[i]->population;
        }
    }

    int tour_bonus = (MAX_TURNS - game->turn) * 10;
    if (tour_bonus < 0) tour_bonus = 0;

    return (total_techs * 100) + (active_cities * 50) + (total_pop * 20) + tour_bonus;

}
