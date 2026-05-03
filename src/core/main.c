#include <stdio.h>
#include <stdlib.h> 
#include <string.h>
#include <stdbool.h>
#include <time.h>
#include <string.h>

#include "../../include/core/game.h"
#include "../../include/core/map.h"
#include "../../include/core/types.h"
#include "../../include/core/physics.h"
#include "../../include/core/save.h"
#include "../../include/core/tech.h"
#include "../../include/core/ressources.h"
#include "../../include/core/config.h"

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

// --- MENU PRINCIPAL ---
// Renvoie 1 si le joueur veut jouer, 0 s'il quitte
static int run_main_menu(void) {
    char choice;
    
    while (1) {
        printf("\n\n");
        printf("  ╔═════════════════════════════════════════════╗\n");
        printf("  ║                                             ║\n");
		printf("  ║ -=*=-  L E S   I N C I V I L I S É S  -=*=- ║\n");
        printf("  ║                                             ║\n");
        printf("  ╠═════════════════════════════════════════════╣\n");
        printf("  ║                                             ║\n");
		printf("  ║   [1] + Nouvelle Partie                     ║\n");
        printf("  ║   [2] > Charger une partie                  ║\n");
        printf("  ║   [3] i Règles et Commandes                 ║\n");
        printf("  ║   [4] # Options (Bientôt)                   ║\n");
        printf("  ║                                             ║\n");
  		printf("  ║   [x] <- Quitter                            ║\n");
        printf("  ║                                             ║\n");
        printf("  ╚═════════════════════════════════════════════╝\n");
		printf("  Veuillez sélectionner une option pour continuer: ");

        if (scanf(" %c", &choice) != 1) {
            // Sécurité (si la lecture échoue)
            return 0; 
        }

        switch (choice) {
            case '1':
                printf("\n  Chargement de la partie en cours...\n");
                return 1;

            case '2':
                return 2;
            case '4':
                printf("\n Fonctionnalité en cours de développement ça charbonne \n");
                break;

            case '3':
                printf("\n  === RÈGLES DU JEU ===\n");
                printf("  Le but est de faire prospérer votre ville et de survivre.\n");
                printf("  Des barbares apparaîtront et tenteront de vous détruire.\n\n");
				// RÈGLES PLUS PRÉCISE SANS FAIRE DE TUNNEL
                printf("  === COMMANDES ===\n");
                printf("  z/q/s/d : Déplacer votre unité (Haut/Gauche/Bas/Droite)\n");
                printf("  n       : Finir votre tour (récupérer les PM)\n");
                printf("  x       : Quitter la partie en cours\n");
                printf("  Appuyez sur Entrée pour revenir au menu...\n");
                
				// On attend que le joueur tappe entrée
                int c;
                while ((c = getchar()) != '\n' && c != EOF); // Vide le buffer
                getchar(); // Attend Entrée
                break;

            case 'x':
            case 'X':
                printf("\n  Arrêt du jeu. À bientôt !\n");
                return 0;

            default:
                printf("\n  \033[1;31m Choix invalide. Veuillez sélectionner une option valide.\033[0m\n");
                break;
        }
    }
}

// Affichage d'état : j'utilise le hook externe si dispo, sinon affichage console.
static void print_state_maxence_stub(const GameState *game)
{
    if (display_state_maxence) {
        display_state_maxence(&game->map, &game->player, game->cities[0],
            game->turn, game->player_unit_count, game->barbarian_count);
        return;
    }

    printf("\n=== TOUR %d ===\n", game->turn);
    printf("Ressources: or=%d science=%d\n", game->player.gold, game->player.science);
    printf("Ville: hp=%d pop=%d (%d,%d)\n", game->cities[0]->hp, game->cities[0]->population, game->cities[0]->x, game->cities[0]->y);
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

	// GESTION DU MODE D'AFFICHAGE

    if (strcmp(mode, "sdl") == 0) {
        puts("[CIV] Mode SDL initialise (squelette).\nInstallez SDL2 et branchez l'UI plus tard.");
        return 0;
    } else if (strcmp(mode, "cli") != 0) {
        fprintf(stderr, "Mode invalide: %s\n", mode);
        return 1;
    }


    // Initialisation de la configuration de jeu (difficulty presets)
    GameConfig cfg;
    game_config_init_default(&cfg);
    printf("[CIV] Difficulty: %s\n", difficulty_to_string(cfg.difficulty));

    // MENU PRINCIPAL
    int state = run_main_menu();
    if (state == 0) return 0;
	

	// DEBUT DU JEU

    GameState game;
    memset(&game, 0, sizeof(GameState)); // Sécurité : on initialise tout à 0 pour éviter les valeurs indéterminées.
    if (state == 1) {
        init_game(&game, width, height, seed);
        printf("[CIV] Nouvelle partie; Generation avec la graine : %d\n", seed);
    } else if(state == 2) {
        if (!load_game("savegame.sav", &game)) {
            printf("Échec du chargement de la partie. Démarrage d'une nouvelle partie.\n");
            init_game(&game, width, height, seed);
            printf("[CIV] Nouvelle partie; Generation avec la graine : %d\n", seed);
        } 

    }

    puts("[CIV] Boucle principale lancée (prototype).\n");

    int city_id_counter=1;

    // Boucle centrale demandée dans le ticket.
    while (!game.game_over) {
        // On calcule ce que le joueur a le droit de voir(Brouillard de Guerre)
        update_fog_of_war(&game);
        display_map_color(&game.map);

        // 1. Affichage d'état (hook externe ou console)
        print_state_maxence_stub(&game);
        
        printf("\n[Or : %d] Action ? 1: Deplacer unite | 2: Recruter | 3: Passer le tour | t: Technologies | f : fonder une ville | v : gérer les villes | s: Sauvegarder | x: Quitter\n", game.player.gold);
        printf("Votre choix : ");
        
        char choice;
        if (scanf(" %c", &choice) != 1) continue;

        switch (choice) {
            case 'x':
            case 'X':
                printf("\nArret du jeu. A bientot !\n");
                game.game_over = 1;
                continue;

            case 'f':
                printf("\nUnites disponibles :\n");
                for (int i = 0; i < game.player_unit_count; i++) {
                    Unit *u = game.player_units[i];
                    if (u && u->hp > 0) {
                        const char* nom_unite = (u->type == UNIT_WARRIOR) ? "Guerrier" : "Colon";
                        printf("[%d] %s en (%d,%d) - %d/%d PM\n", i, nom_unite, u->x, u->y, u->pm, u->pm_max);
                    }
                }
        
                printf("Quelle unite selectionner ? (0 a %d, ou x pour annuler) : ", game.player_unit_count - 1);
                char idx_choice;
                if (scanf(" %c", &idx_choice) != 1 || idx_choice == 'x') {
                    continue;
                }

                int unit_idx = idx_choice - '0';
                if (unit_idx < 0 || unit_idx >= game.player_unit_count) {
                    printf("\n\033[1;31mIndex invalide.\033[0m\n");
                    continue;
                }

                Unit *mon_unite = game.player_units[unit_idx];

                // Sécurité : on vérifie que l'unité existe et est en vie
                if (mon_unite == NULL || mon_unite->hp <= 0) {
                printf("\n\033[1;31mCette unite est morte ou n'existe pas.\033[0m\n");
                continue;
                }

                if (mon_unite->pm <= 0) {
                    printf("\n\033[1;33mCette unite n'a plus de PM. Choisissez-en une autre.\033[0m\n");
                    continue;
                }

                if (found_city(mon_unite, &game.map, &city_id_counter, game.cities, &game.city_count)) {
                    game.player_units[unit_idx] = NULL; 
                }
                
                continue;

            case 'v':
            case 'V':
                if (game.city_count == 0) {
                    printf("\n\033[1;31mVous ne possedez aucune ville !\033[0m\n");
                    continue;
                }

                printf("\n=== VOS VILLES ===\n");
                for (int i = 0; i < game.city_count; i++) {
                    if (game.cities[i] && game.cities[i]->hp > 0) {
                        printf("[%d] Ville en (%d, %d) - Pop: %d - Prod: %d\n", 
                            i, game.cities[i]->x, game.cities[i]->y, 
                            game.cities[i]->population, game.cities[i]->production_stored);
                    }
                }
                
                printf("\nQuelle ville voulez-vous gerer ? (0 a %d, ou x pour annuler) : ", game.city_count - 1);
                char city_choice;
                if (scanf(" %c", &city_choice) != 1 || city_choice == 'x') {
                    continue;
                }

                int city_idx = city_choice - '0';
                
                // Sécurité : on vérifie que l'index est valide et que la ville existe
                if (city_idx < 0 || city_idx >= game.city_count || game.cities[city_idx] == NULL || game.cities[city_idx]->hp <= 0) {
                    printf("\n\033[1;31mIndex de ville invalide.\033[0m\n");
                    continue;
                }

                // On envoie la ville sélectionnée à la fonction de gestion
                control_city_project(game.cities[city_idx]);
                continue;
                
            case 't':
            case 'T':
                printf("\n === ARBRE TECHNOLOGIQUE ===\n");
                printf("Cagnotte de science : %d\n", game.player.science);
                if (game.player.current_research != TECH_NONE) {
                    printf("Recherche en cours : %s (%d / %d)\n", TECH_CATALOG[game.player.current_research].name, game.player.science, TECH_CATALOG[game.player.current_research].cost);
                } else {
                    printf("Aucune recherche en cours.\n");
                }
                printf("\nTechnologies disponibles à la recherche :\n");
                int tech_found = 0;
                for (int i = 0; i < MAX_TECHNOLOGIES; i++) {
                    // On affiche seulement ce qu'on a le droit de rechercher
                    if (can_research(&game.player, i)) {
                        printf("[%d] %s (Cout: %d Science)\n", i, TECH_CATALOG[i].name, TECH_CATALOG[i].cost);
                        tech_found = 1; 
                    }
                }
                if (!tech_found) {
                    printf("Aucune technologie disponible à la recherche pour le moment.\n");
                }
                printf("[x] Annuler\n");
                printf("Quelle technologie souhaitez-vous rechercher ? ");
                char tech_choice;
                if (scanf(" %c", &tech_choice) != 1 || tech_choice == 'x') {
                    continue;
                }
                int tech_idx = tech_choice - '0';
                if (tech_idx >= 0 && tech_idx < MAX_TECHNOLOGIES) {
                    start_research(&game.player, tech_idx);
                } else {
                    printf("\n\033[1;31mChoix invalide.\033[0m\n");
                }
                continue;

            case 's':
            case 'S':
                printf("\nSauvegarde en cours...\n");
                save_game("savegame.sav", &game);
                continue;

            case '3':
            case 'n':
                // 5. Le joueur a fini de jouer, l'ordinateur (Barbares d'Armel) joue son tour
                next_turn(&game);
                if (game.turn >= nb_turns || game.cities[0]->hp <= 0) {
                    game.game_over = 1;
                }
                continue;

            case '2':
                printf("\nQuelle unité souhaitez-vous recruter ?\n");
                printf("[1] Guerrier (10 Or) - Force: 3, PM: 3\n");
                printf("[2] Colon (20 Or) - Fonde une ville, PM: 2\n");
                printf("[x] Annuler\n");
                printf("Votre choix : ");

                char rec_choice;
                if (scanf(" %c", &rec_choice) != 1 || rec_choice == 'x') {
                    continue;
                }

                int cost = 0;
                int type = 0;

                if (rec_choice == '1') {
                    cost = 10;
                    type = UNIT_WARRIOR;
                } else if (rec_choice == '2') {
                    cost = 20;
                    type = UNIT_SETTLER;
                } else {
                    printf("\n\033[1;31mChoix invalide.\033[0m\n");
                    continue;
                }

                if (game.player.gold >= cost) {
                    if (game.player_unit_count < 8) { 
                        Tile *city_tile = &game.map.tiles[game.cities[0]->y][game.cities[0]->x];
                        
                        if (city_tile->unit == NULL) {
                            Unit *new_unit = malloc(sizeof(Unit));
                            if (new_unit) {
                                new_unit->type = type;
                                new_unit->x = game.cities[0]->x;
                                new_unit->y = game.cities[0]->y;
                                new_unit->owner = &game.player;
                                
                                if (type == UNIT_WARRIOR) {
                                    new_unit->hp = 15; new_unit->hp_max = 15;
                                    new_unit->attack = 3; new_unit->defense = 2;
                                    new_unit->pm = 3; new_unit->pm_max = 3;
                                } else { 
                                    new_unit->hp = 10; new_unit->hp_max = 10;
                                    new_unit->attack = 0; new_unit->defense = 1;
                                    new_unit->pm = 2; new_unit->pm_max = 2;
                                }
                                
                                game.player_units[game.player_unit_count++] = new_unit;
                                city_tile->unit = new_unit;
                                
                                game.player.gold -= cost;
                                printf("\n\033[1;32m[+] Unite recrutee avec succes sur la ville !\033[0m\n");
                            }
                        } else {
                            printf("\n\033[1;31m[!] La ville est deja occupee. Deplacez l'unite presente d'abord.\033[0m\n");
                        }
                    } else {
                        printf("\n\033[1;31m[!] Vous avez atteint la limite d'unites actives.\033[0m\n");
                    }
                } else {
                    printf("\n\033[1;31m[!] Or insuffisant. Il vous faut %d Or.\033[0m\n", cost);
                }
                continue;

            case '1':
                break; // Sort du match pour lancer la logique de deplacement juste en dessous

            default:
                printf("\n\033[1;31mAction invalide.\033[0m\n");
                continue;
        }

        printf("\nUnites disponibles :\n");
        for (int i = 0; i < game.player_unit_count; i++) {
            Unit *u = game.player_units[i];
            if (u && u->hp > 0) {
                const char* nom_unite = (u->type == UNIT_WARRIOR) ? "Guerrier" : "Colon";
                printf("[%d] %s en (%d,%d) - %d/%d PM\n", i, nom_unite, u->x, u->y, u->pm, u->pm_max);
            }
        }
        
        printf("Quelle unite selectionner ? (0 a %d, ou x pour annuler) : ", game.player_unit_count - 1);
        char idx_choice;
        if (scanf(" %c", &idx_choice) != 1 || idx_choice == 'x') {
            continue;
        }

        int unit_idx = idx_choice - '0';
        if (unit_idx < 0 || unit_idx >= game.player_unit_count) {
            printf("\n\033[1;31mIndex invalide.\033[0m\n");
            continue;
        }

        Unit *mon_unite = game.player_units[unit_idx];

        // Sécurité : on vérifie que l'unité existe et est en vie
        if (mon_unite == NULL || mon_unite->hp <= 0) {
            printf("\n\033[1;31mCette unite est morte ou n'existe pas.\033[0m\n");
            continue;
        }

        if (mon_unite->pm <= 0) {
            printf("\n\033[1;33mCette unite n'a plus de PM. Choisissez-en une autre.\033[0m\n");
            continue;
        }

        // 2. Pause et lecture de la commande clavier
        printf("Direction (z:Haut / q:Gauche / s:Bas / d:Droite) : ");
        char action;
        if (scanf(" %c", &action) != 1) continue;

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
            if (can_move(mon_unite, target_tile, &game.player)) {

                int has_attacked = 0; // Permet de savoir si on a donné un coup pour détruire le PM


                // A1. Combat contre une unité ennemie (Barbare)
                if (target_tile->unit && target_tile->unit->owner != mon_unite->owner) {
                    printf("\n Combat engagé contre le barbare ! \n");
                    printf("-> Avant : Toi (%d PV) vs Barbare (%d PV)\n", mon_unite->hp, target_tile->unit->hp);
                    resolve_combat(mon_unite, target_tile->unit);
                    printf("-> Apres : Toi (%d PV) vs Barbare (%d PV)\n", mon_unite->hp, target_tile->unit->hp);

                    // VERDICT DU COMBAT

                    if(target_tile->unit->hp <= 0) {
                        printf("VICTOIRE ! Le barbare est mort !\n");
                        target_tile->unit = NULL; //On vide la place de parking du Barbare
                    }
                }

                // A2. Combat contre une ville ennemie (Barbare)
                else if (target_tile->city && target_tile->city->owner != mon_unite->owner) {
                    has_attacked = 1;
                    printf("\n Attaque de la ville ennemie ! \n");
                    printf("-> Avant : Toi (%d PV) vs Ville (%d PV)\n", mon_unite->hp, target_tile->city->hp);
                    resolve_combat_city(mon_unite, target_tile->city);
                    printf("-> Apres : Toi (%d PV) vs Ville (%d PV)\n", mon_unite->hp, target_tile->city->hp);

                    if (target_tile->city->hp <= 0) {
                        printf("VICTOIRE ! La ville a été rasée !\n");
                        free(target_tile->city);
                        target_tile->city = NULL; //On vide la place de parking de la ville
                    }
                }

                // A3. Pillage d'un camp barbare
                else if (target_tile->camp != NULL && target_tile->camp->destroyed == 0) {
                    printf("\n Vous avez détruit un campement barbare ! \n");
                    target_tile->camp->destroyed = 1; // On détruit le camp(pas de combat)
                }

                // VERDICT DU MOUVEMENT APRÈS LES COMBATS




                if (mon_unite->hp <= 0) {
                    printf(" DEFAITE ! Votre unité a été massacre ... \n");
                    game.map.tiles[mon_unite->y][mon_unite->x].unit = NULL; // on efface notre joueur
                    game.game_over = 1;
                }
                
                // B. S'il n'y a pas d'ennemi, on se déplace normalement
                else if(target_tile->unit == NULL && target_tile->city == NULL) {
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

                // C. L'obstacle est toujours lè(l'ennemi a surcécu, le mouvement est annulé)
                else if (has_attacked) {
                    mon_unite->pm--; // Même si on a attaqué, le mouvement est annulé mais on perd quand même le PM
                    puts("\n La cible a survécu à votre attaque, Déplacement annulé mais PM consommé ! ");
                }
            } else {
                puts("\nDéplacement impossible (pas assez de PM ou terrain bloque) !");
            }
        } else {
            puts("\n Tu essaies de sortir de la carte ");
        }
    }
    

    puts("\nFin de partie (prototype).\n");
    cleanup_game(&game);
    return 0;
}
