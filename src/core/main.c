#include <stdio.h>
#include <stdlib.h> 
#include <string.h>
#include <stdbool.h>
#include <time.h>

#include "../../include/core/game.h"
#include "../../include/core/map.h"
#include "../../include/core/types.h"
#include "../../include/core/physics.h"

#define MAX_TURNS 200

static void print_help(void)
{
    puts("Usage: ./bin/civ [options]");
    puts("Options:");
    puts("  --help                 Affiche cette aide");
    puts("  --mode MODE | -m MODE  MODE = cli ou sdl");
    puts("  --seed N    | -s N    Graine de génération de la carte (aléatoire par défaut)");
    puts("  --width W   | -W W    Largeur de la carte en tuiles (par défaut: 50)");
    puts("  --height H  | -H H    Hauteur de la carte en tuiles (par défaut: 30)");
    puts("  --turns N   | -t N    Nombre de tours maximum (par défaut: 200)");
    puts("  --barbarians N | -b N Nombre de campements barbares initiaux (par défaut: 3)");
}

// Affichage d'état : j'utilise le hook externe si dispo, sinon affichage console.
static void print_state_maxence_stub(const GameState *game)
{
    if (display_state_maxence) {
        display_state_maxence(&game->map, &game->player, &game->city,
            game->turn, game->player_unit_count, game->barbarian_count);
        return;
    }

    printf("\n=== TOUR %d ===\n", game->turn);
    printf("Ressources: or=%d science=%d\n", game->player.gold, game->player.science);
    printf("Ville: hp=%d pop=%d (%d,%d)\n", game->city.hp, game->city.population, game->city.x, game->city.y);
    printf("Unités joueur: %d | Barbares: %d\n", game->player_unit_count, game->barbarian_count);
}

int main(int argc, char **argv) {
    const char *mode = "cli";
    int width = 50;
    int height = 30;
    int seed = (int)time(NULL);
    bool is_seed = false;
    int nb_turns = 200;
    int nb_barbarians = 3;

    // Parsing initial

    for (int i = 1; i < argc; ++i) {
        if (strcmp(argv[i], "--help") == 0) {
            print_help();
            return 0;
        }

        if ((strcmp(argv[i], "--mode") == 0 || strcmp(argv[i], "-m") == 0) && i + 1 < argc) {
            mode = argv[++i];
        } else if ((strcmp(argv[i], "--seed") == 0 || strcmp(argv[i], "-s") == 0) && i + 1 < argc) {
            seed = atoi(argv[++i]);
            is_seed = true;
        } else if ((strcmp(argv[i], "--width") == 0 || strcmp(argv[i], "-W") == 0) && i + 1 < argc) {
            width = atoi(argv[++i]);
        } else if ((strcmp(argv[i], "--height") == 0 || strcmp(argv[i], "-H") == 0) && i + 1 < argc) {
            height = atoi(argv[++i]);
        } else if ((strcmp(argv[i], "--turns") == 0 || strcmp(argv[i], "-t") == 0) && i + 1 < argc) {
            nb_turns = atoi(argv[++i]);
        } else if ((strcmp(argv[i], "--barbarians") == 0 || strcmp(argv[i], "-b") == 0) && i + 1 < argc) {
            nb_barbarians = atoi(argv[++i]);
        } else {
            fprintf(stderr, "\033[1;31m[Erreur]\033[0m Option invalide: %s\n", argv[i]);
            print_help();
            return 1;
        }
    }

	// Génération de la seed si aucune n'est donnée par le joueur
	if (!is_seed) {
		seed = (int)time(NULL);
	}

	(void)nb_barbarians; // pour éviter les erreurs de compilations avec les variables non utilisées

// ----------------- AFFICHAGE SDL -----------------

    if (strcmp(mode, "sdl") == 0) {
        puts("[CIV] Mode SDL initialise (squelette).\nInstallez SDL2 et branchez l'UI plus tard.");
        return 0;
    } else if (strcmp(mode, "cli") != 0) {
        fprintf(stderr, "Mode invalide: %s\n", mode);
        return 1;
    }

    GameState game;
    init_game(&game, width, height, seed);

    printf("[CIV] Generation avec la graine : %d\n", seed);
    puts("[CIV] Boucle principale lancée (prototype).\n");

    // Boucle centrale demandée dans le ticket.
    while (!game.game_over) {

        display_map_color(&game.map);

        // 1. Affichage d'état (hook externe ou console)
        print_state_maxence_stub(&game);
        
        Unit *mon_unite = game.player_units[0];

        // Sécurité : on vérifie que l'unité existe et est en vie

        if (mon_unite == NULL || mon_unite->hp <= 0) {
            puts("Votre unité est morte ou n'existe pas. Fin de partie.");
            break;
        }

        // 2. Pause et lecture de la commande clavier
        printf("\n[Tour %d] Commande (z/q/s/d pour déplacer, x = Quitter): ", game.turn);
        char action;
        if (scanf(" %c", &action) != 1 || action == 'x') {
            if (action == 'x') printf("Fin de partie demandée par le joueur.\n");
            break;
        }

        // 3. Calcul des futures coordonnées selon la touche pressée
        int target_x = mon_unite->x;
        int target_y = mon_unite->y;

        if (action == 'z') target_y -= 1;   //haut
        else if (action == 's') target_y += 1;  //Bas
        else if (action == 'q') target_x -= 1;   // Gauche
        else if (action == 'd') target_x += 1;  // Droite
        else {
            printf("Touche invalide. Utilisez z/q/s/d pour déplacer ou x pour quitter.\n");
            continue;
        }

        // 4. Logique de dépacement et application de ma physique (can_move, resolve_combat, etc.)

        // Vérification des coordonnées de la carte pour éviter des erreurs d'accès mémoire
        if (target_x >= 0 && target_x < game.map.width && target_y >= 0 && target_y < game.map.height) {

            // On cible la tuile d'arrivée 
            Tile *target_tile = &game.map.tiles[target_y][target_x];

            // Appel de la physique du jeu
            if (can_move(mon_unite, target_tile)) {

                // A. S'il y a un ennemi sur la case cible: Fight !
                if (target_tile->unit && target_tile->unit->owner != mon_unite->owner) {
                    printf("\n Combat engagé ! \n");
                    printf("-> Avant : Toi (%d PV) vs Barbare (%d PV)\n", mon_unite->hp, target_tile->unit->hp);
                    resolve_combat(mon_unite, target_tile->unit);
                    printf("-> Apres : Toi (%d PV) vs Barbare (%d PV)\n", mon_unite->hp, target_tile->unit->hp);

                    // VERDICT DU COMBAT

                    if(target_tile->unit->hp <= 0) {
                        printf("VICTOIRE ! Le barbare est mort !\n");
                        target_tile->unit = NULL; //On vide la place de parking du Barbare
                    }
                    if (mon_unite->hp <= 0) {
                        printf(" DEFAITE ! Votre unité a été massacre ... \n");
                        game.map.tiles[mon_unite->y][mon_unite->x].unit = NULL; // on efface notre joueur
                        game.game_over = 1;
                    }
                }
                // B. S'il n'y a pas d'ennemi, on se déplace normalement
                else if(target_tile->unit == NULL) {
                    // 1. On "vide" l'ancienne tuile de la carte
                    game.map.tiles[mon_unite->y][mon_unite->x].unit = NULL;
                    // 2. On met à jour les coordonnées de l'unité
                    mon_unite->x = target_x;
                    mon_unite->y = target_y;
                    mon_unite->pm--;

                    // 3. On place le pointeur de l'unité sur la nouvelle tuile de la carte 
                    target_tile->unit = mon_unite;

                    puts("\nDéplacement réussi !\n");
                }
            } else {
                puts("\nDéplacement impossible (pas assez de PM ou terrain bloque) !");
            }
        } else {
            puts("\n Tu essaies de sortir de la carte ");
        }

        // 5. Le joueur a fini de jouer, l'ordinateur (Barbares d'Armel) joue son tour
        next_turn(&game);

        if (game.turn >= nb_turns || game.city.hp <= 0) {
            game.game_over = 1;
        }
    }

    puts("\nFin de partie (prototype).\n");
    cleanup_game(&game);
    return 0;
}
