#define _XOPEN_SOURCE_EXTENDED
#include <stdio.h>
#include <stdlib.h> 
#include <string.h>
#include <stdbool.h>
#include <time.h>
#include <string.h>
#include <ncurses.h>
#include <locale.h>

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
    puts("  --difficulty N | -d N  N = facile, normal ou difficile");
    puts("  --production-multiplier N | -p N  Multiplie la production de base");
    puts("  --growth-multiplier N     | -g N  Multiplie le seuil de croissance");
    puts("  --barbarian-spawn-multiplier N | -B N  Multiplie la fréquence des barbares");
    puts("  --famine N | -f N  N = on/off, true/false, 1/0");
    puts("  --seed N    | -s N    Graine de génération de la carte (aléatoire par défaut)");
    puts("  --width W   | -W W    Largeur de la carte en tuiles (par défaut: 50)");
    puts("  --height H  | -H H    Hauteur de la carte en tuiles (par défaut: 30)");
    puts("  --turns N   | -t N    Nombre de tours maximum (par défaut: 200)");
    puts("  --barbarians N | -b N Nombre de campements barbares initiaux (par défaut: 3)");
}

// Gestion des options du menu principal
static void run_options_menu(int max_y, int max_x, int *diff_level, bool *fow) {
    (void)max_x;
    int opt_highlight = 1;
    while (true) {  
        clear();
        attron(COLOR_PAIR(1)); box(stdscr, 0, 0); 
        attroff(COLOR_PAIR(1));
        attron(COLOR_PAIR(3) | A_BOLD); 
        mvprintw(2, 4, "=== OPTIONS DE JEU ==="); 
        attroff(COLOR_PAIR(3) | A_BOLD);
        
        const char* diff_str = (*diff_level == 0) ? "Facile" : (*diff_level == 1) ? "Normal" : "Difficile";
        const char* fow_str = (*fow) ? "Active" : "Desactive";
        
        char opt1[64], opt2[64], opt3[64];
        sprintf(opt1, "  [1] Difficulte         : < %s >  ", diff_str);
        sprintf(opt2, "  [2] Brouillard de Guerre : < %s >  ", fow_str);
        sprintf(opt3, "  [3] Retour au menu principal  ");
        
        const char *options[] = { opt1, opt2, opt3 };
        
        for(int i=0; i<3; i++) {
            if(opt_highlight == i+1) {
                attron(COLOR_PAIR(3) | A_REVERSE | A_BOLD);
                mvprintw(6 + i*2, 4, "%s", options[i]);
                attroff(COLOR_PAIR(3) | A_REVERSE | A_BOLD);
            } else {
                attron(COLOR_PAIR(30));
                mvprintw(6 + i*2, 4, "%s", options[i]);
                attroff(COLOR_PAIR(30));
            }
        }
        
        attron(COLOR_PAIR(40));
        mvprintw(max_y - 2, 2, " Utilisez Z/S pour naviguer, Entree ou Clic pour modifier les options ");
        attroff(COLOR_PAIR(40));
        refresh();
        
        int c = getch();
        if (c == KEY_UP || c == 'z' || c == 'Z') {
            opt_highlight = (opt_highlight == 1) ? 3 : opt_highlight - 1;
        } else if (c == KEY_DOWN || c == 's' || c == 'S') {
            opt_highlight = (opt_highlight == 3) ? 1 : opt_highlight + 1;
        } else if (c == 10) {
            if (opt_highlight == 1) *diff_level = (*diff_level + 1) % 3;
            else if (opt_highlight == 2) *fow = !(*fow);
            else if (opt_highlight == 3) return;
        } else if (c == KEY_MOUSE) {
            MEVENT event;
            if (getmouse(&event) == OK && (event.bstate & BUTTON1_CLICKED)) {
                for(int i=0; i<3; i++) {
                    if (event.y == 6 + i*2 && event.x >= 4 && event.x <= 4 + (int)strlen(options[i])) {
                        if (i == 0) *diff_level = (*diff_level + 1) % 3;
                        else if (i == 1) *fow = !(*fow);
                        else if (i == 2) return;
                    }
                }
            }
        }
    }
}

// --- MENU PRINCIPAL ---
static int run_main_menu(int *diff_level, bool *fow) {
    int choice;
    int highlight = 1; 
    int n_choices = 6;
    const char *choices[] = {
        "  [1] NOUVELLE PARTIE  ",
        "  [2] CHARGER PARTIE   ",
        "  [3] REGLES & COMMANDES",
        "  [4] OPTIONS          ",
        "  [5] Historique des scores  ",
        "  [x] QUITTER LE JEU   "
    };

    while (1) {
        clear(); 
        int my, mx;
        getmaxyx(stdscr, my, mx); 

        attron(COLOR_PAIR(1)); 
        box(stdscr, 0, 0);      
        attroff(COLOR_PAIR(1));

        int title_y = 2;
        
        // ASCII ART : TITRE COMPLET CENTRE ET EMPILE POUR RENTRER DANS L'ECRAN
        attron(COLOR_PAIR(20) | A_BOLD); 
        mvprintw(title_y,     (mx-25)/2, "  _     _____ _____  ");
        mvprintw(title_y + 1, (mx-25)/2, " | |   | ____/ ___|  ");
        mvprintw(title_y + 2, (mx-25)/2, " | |   |  _| \\___ \\  ");
        mvprintw(title_y + 3, (mx-25)/2, " | |___| |___ ___) | ");
        mvprintw(title_y + 4, (mx-25)/2, " |_____|_____|____/  ");
        
        mvprintw(title_y + 6, (mx-61)/2, " ___ _   _  ____ _____     _____ _     ___ ____  _____ ____  ");
        mvprintw(title_y + 7, (mx-61)/2, "|_ _| \\ | |/ ___|_ _\\ \\   / /_ _| |   |_ _/ ___|| ____/ ___| ");
        mvprintw(title_y + 8, (mx-61)/2, " | ||  \\| | |    | | \\ \\ / / | || |    | |\\___ \\|  _| \\___ \\ ");
        mvprintw(title_y + 9, (mx-61)/2, " | || |\\  | |___ | |  \\ V /  | || |___ | | ___) | |___ ___) |");
        mvprintw(title_y + 10,(mx-61)/2, "|___|_| \\_|\\____|___|  \\_/  |___|_____|___|____/|_____|____/ ");
        attroff(COLOR_PAIR(20) | A_BOLD);

        int menu_y = my / 2 + 2;
        int option_center_x = mx / 2;

        // --- DESSINS DECORATIFS ---

        // Gauche : Epée de Chevalier
        attron(COLOR_PAIR(5) | A_BOLD);
        int art_left_x = option_center_x - 30;
        mvprintw(menu_y - 2, art_left_x, "      />");
        mvprintw(menu_y - 1, art_left_x, "     /<");
        mvprintw(menu_y,     art_left_x, " [|||]OOOOOOOOO>");
        mvprintw(menu_y + 1, art_left_x, " [|||]OOOOOOOOO>");
        mvprintw(menu_y + 2, art_left_x, "     \\<");
        mvprintw(menu_y + 3, art_left_x, "      \\>");
        attroff(COLOR_PAIR(5) | A_BOLD);

        // Droite : Bouclier Lourd
        attron(COLOR_PAIR(20) | A_BOLD);
        int art_right_x = option_center_x + 16;
        mvprintw(menu_y - 3, art_right_x, "   ______");
        mvprintw(menu_y - 2, art_right_x, "  / |  | \\");
        mvprintw(menu_y - 1, art_right_x, " |  |  |  |");
        mvprintw(menu_y,     art_right_x, " |--+--+--|");
        mvprintw(menu_y + 1, art_right_x, " |  |  |  |");
        mvprintw(menu_y + 2, art_right_x, "  \\ |  | /");
        mvprintw(menu_y + 3, art_right_x, "   \\    /");
        mvprintw(menu_y + 4, art_right_x, "    \\__/");
        attroff(COLOR_PAIR(20) | A_BOLD);

        // --- OPTIONS ---
        for(int i = 0; i < n_choices; i++) {
            int x_pos = (mx - strlen(choices[i])) / 2;
            int y_pos = menu_y + (i * 2);

            if(highlight == i + 1) {
                attron(COLOR_PAIR(3) | A_REVERSE | A_BOLD); // Vert inversé pour sélection
                mvprintw(y_pos, x_pos, "%s", choices[i]);
                attroff(COLOR_PAIR(3) | A_REVERSE | A_BOLD);
            } else {
                attron(COLOR_PAIR(30)); // Blanc pour les autres choix
                mvprintw(y_pos, x_pos, "%s", choices[i]);
                attroff(COLOR_PAIR(30));
            }
        }

        attron(COLOR_PAIR(40)); 
        mvprintw(my - 2, 2, " Utilisez Z/S ou Fleches pour naviguer, Entree ou Clic pour valider ");
        attroff(COLOR_PAIR(40));

        refresh();

        choice = getch();

        switch (choice) {
            case KEY_UP:
            case 'z':
            case 'Z':
                highlight = (highlight == 1) ? n_choices : highlight - 1;
                break;
            case KEY_DOWN:
            case 's':
            case 'S':
                highlight = (highlight == n_choices) ? 1 : highlight + 1;
                break;
            case 10: 
                if (highlight == 1) return 1;
                if (highlight == 2) return 2;
                if (highlight == 3) { 
                    clear();
                    attron(COLOR_PAIR(1)); box(stdscr, 0, 0); attroff(COLOR_PAIR(1));
                    attron(COLOR_PAIR(3) | A_BOLD);
                    mvprintw(2, 4, "=== REGLES DE L'EMPIRE ===");
                    attroff(COLOR_PAIR(3) | A_BOLD);
                    attron(COLOR_PAIR(30));
                    mvprintw(4, 4, "- BUT : Developper votre civilisation et survivre aux Barbares.");
                    mvprintw(5, 4, "- NOURRITURE : Fait croitre la population. Attention a la famine !");
                    mvprintw(6, 4, "- PRODUCTION : Sert a construire des batiments et recruter des unites.");
                    mvprintw(7, 4, "- SCIENCE : Debloque de nouvelles technologies cruciales.");
                    mvprintw(8, 4, "- COMBAT : La force depend de l'unite. Les forets/montagnes ralentissent.");
                    attroff(COLOR_PAIR(30));
                    
                    attron(COLOR_PAIR(4) | A_BOLD);
                    mvprintw(10, 4, "=== COMMANDES PRINCIPALES ===");
                    attroff(COLOR_PAIR(4) | A_BOLD);
                    attron(COLOR_PAIR(30));
                    mvprintw(12, 4, "[Clic]    : Selectionner une unite ou une ville sur la carte");
                    mvprintw(13, 4, "[z/q/s/d] : Deplacer votre unite selectionnee");
                    mvprintw(14, 4, "[f]       : Fonder une ville (necessite un Colon)");
                    mvprintw(15, 4, "[c] / [v] : Gerer la ville selectionnee (Construction/Recrutement)");
                    mvprintw(16, 4, "[t]       : Ouvrir l'arbre des Technologies");
                    mvprintw(17, 4, "[n]       : Finir votre tour (recuperer les PM)");
                    mvprintw(18, 4, "[Maj+S]   : Sauvegarder la partie en cours");
                    mvprintw(19, 4, "[x]       : Quitter le jeu");
                    mvprintw(22, 4, "Appuyez sur une touche pour revenir...");
                    attroff(COLOR_PAIR(30));
                    refresh();
                    getch();
                }
                if (highlight == 4) {
                    run_options_menu(my, mx, diff_level, fow);
                    break;
                }
                if (highlight == 5) {
                    display_history_ncurses();
                    break;
                }

                if (highlight == 6) return 0;
                break;
            case KEY_MOUSE: {
                MEVENT event;
                if (getmouse(&event) == OK) {
                    if (event.bstate & BUTTON1_CLICKED) {
                        for(int i = 0; i < n_choices; i++) {
                            int opt_x = (mx - (int)strlen(choices[i])) / 2;
                            int opt_y = menu_y + (i * 2);
                            
                            // Si le clic correspond aux coordonnées de l'option i
                            if (event.y == opt_y && event.x >= opt_x && event.x <= opt_x + (int)strlen(choices[i])) {
                                highlight = i + 1;
                                
                                // On déclenche l'action associée au clic immédiatement
                                if (highlight == 1) return 1;
                                if (highlight == 2) return 2;
                                if (highlight == 3) {
                                    clear();
                                    attron(COLOR_PAIR(1)); box(stdscr, 0, 0); attroff(COLOR_PAIR(1));
                                    attron(COLOR_PAIR(3) | A_BOLD);
                                    mvprintw(2, 4, "=== REGLES DE L'EMPIRE ===");
                                    attroff(COLOR_PAIR(3) | A_BOLD);
                                    attron(COLOR_PAIR(30));
                                    mvprintw(4, 4, "- BUT : Developper votre civilisation et survivre aux Barbares.");
                                    mvprintw(5, 4, "- NOURRITURE : Fait croitre la population. Attention a la famine !");
                                    mvprintw(6, 4, "- PRODUCTION : Sert a construire des batiments et recruter des unites.");
                                    mvprintw(7, 4, "- SCIENCE : Debloque de nouvelles technologies cruciales.");
                                    mvprintw(8, 4, "- COMBAT : La force depend de l'unite. Les forets/montagnes ralentissent.");
                                    attroff(COLOR_PAIR(30));
                                    
                                    attron(COLOR_PAIR(4) | A_BOLD);
                                    mvprintw(10, 4, "=== COMMANDES PRINCIPALES ===");
                                    attroff(COLOR_PAIR(4) | A_BOLD);
                                    attron(COLOR_PAIR(30));
                                    mvprintw(12, 4, "[Clic]    : Selectionner une unite ou une ville sur la carte");
                                    mvprintw(13, 4, "[z/q/s/d] : Deplacer votre unite selectionnee");
                                    mvprintw(14, 4, "[f]       : Fonder une ville (necessite un Colon)");
                                    mvprintw(15, 4, "[c] / [v] : Gerer la ville selectionnee (Construction/Recrutement)");
                                    mvprintw(16, 4, "[t]       : Ouvrir l'arbre des Technologies");
                                    mvprintw(17, 4, "[n]       : Finir votre tour (recuperer les PM)");
                                    mvprintw(18, 4, "[Maj+S]   : Sauvegarder la partie en cours");
                                    mvprintw(20, 4, "[x]       : Quitter le jeu");
                                    mvprintw(22, 4, "Appuyez sur une touche pour revenir...");
                                    attroff(COLOR_PAIR(30));
                                    refresh();
                                    getch();
                                    break; // On casse la boucle for pour redessiner le menu principal
                                }
                                if (highlight == 4) {
                                    run_options_menu(my, mx, diff_level, fow);
                                    break;
                                }
                                if (highlight == 5) {
                                    display_history_ncurses();
                                    break;
                                }
                                if (highlight == 6) {
                                    return 0;
                                }
                            }
                        }
                    }
                }
                break;
            }
            case '1': return 1;
            case '2': return 2;
            case 'x':
            case 'X': return 0;
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
    (void)game; // Pour éviter l'erreur de paramètre non utilisé avec -Werror
}

static const char *unit_label(UnitType type)
{
    switch (type) {
        case UNIT_WARRIOR:
            return "Guerrier";
        case UNIT_ARCHER:
            return "Archer";
        case UNIT_CAVALIER:
            return "Cavalier";
        case UNIT_SETTLER:
        default:
            return "Colon";
    }
}

int main(int argc, char **argv) {
    setlocale(LC_ALL, "");

    const char *mode = "cli";
    Difficulty difficulty = DIFF_NORMAL;
    int width = 50;
    int height = 30;
    int seed = (int)time(NULL);
    bool is_seed = false;
    int nb_turns = 200;
    int nb_barbarians = 3;
    bool has_production_override = false;
    bool has_growth_override = false;
    bool has_barbarian_spawn_override = false;
    bool has_famine_override = false;
    float production_multiplier_override = 1.0f;
    float growth_multiplier_override = 1.0f;
    float barbarian_spawn_multiplier_override = 1.0f;
    bool famine_override = true;

    int diff_level_ui = 1;
    bool enable_fow_ui = true;

    // Parsing initial

    for (int i = 1; i < argc; ++i) {
        if (strcmp(argv[i], "--help") == 0) {
            print_help();
            return 0;
        }

        if ((strcmp(argv[i], "--mode") == 0 || strcmp(argv[i], "-m") == 0) && i + 1 < argc) {
            mode = argv[++i];
        } else if ((strcmp(argv[i], "--difficulty") == 0 || strcmp(argv[i], "-d") == 0) && i + 1 < argc) {
            Difficulty parsed_difficulty;
            if (!difficulty_from_string(argv[++i], &parsed_difficulty)) {
                fprintf(stderr, "\033[1;31m[Erreur]\033[0m Niveau de difficulte invalide: %s\n", argv[i]);
                print_help();
                return 1;
            }
            difficulty = parsed_difficulty;
        } else if ((strcmp(argv[i], "--production-multiplier") == 0 || strcmp(argv[i], "-p") == 0) && i + 1 < argc) {
            if (!config_parse_float(argv[++i], &production_multiplier_override)) {
                fprintf(stderr, "\033[1;31m[Erreur]\033[0m Multiplicateur de production invalide: %s\n", argv[i]);
                print_help();
                return 1;
            }
            has_production_override = true;
        } else if ((strcmp(argv[i], "--growth-multiplier") == 0 || strcmp(argv[i], "-g") == 0) && i + 1 < argc) {
            if (!config_parse_float(argv[++i], &growth_multiplier_override)) {
                fprintf(stderr, "\033[1;31m[Erreur]\033[0m Multiplicateur de croissance invalide: %s\n", argv[i]);
                print_help();
                return 1;
            }
            has_growth_override = true;
        } else if ((strcmp(argv[i], "--barbarian-spawn-multiplier") == 0 || strcmp(argv[i], "-B") == 0) && i + 1 < argc) {
            if (!config_parse_float(argv[++i], &barbarian_spawn_multiplier_override)) {
                fprintf(stderr, "\033[1;31m[Erreur]\033[0m Multiplicateur barbare invalide: %s\n", argv[i]);
                print_help();
                return 1;
            }
            has_barbarian_spawn_override = true;
        } else if ((strcmp(argv[i], "--famine") == 0 || strcmp(argv[i], "-f") == 0) && i + 1 < argc) {
            if (!config_parse_bool(argv[++i], &famine_override)) {
                fprintf(stderr, "\033[1;31m[Erreur]\033[0m Valeur de famine invalide: %s\n", argv[i]);
                print_help();
                return 1;
            }
            has_famine_override = true;
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

// --- INITIALISATION NCURSES ---
    initscr();              // Démarre ncurses et l'écran principal
    raw();                  // Entrée instantanée (plus besoin de valider avec Entrée)
    keypad(stdscr, TRUE);   // Active les touches spéciales (flèches, etc.)
    noecho();               // Masque les touches tapées par le joueur
    curs_set(0);            // Cache le curseur clignotant
    start_color();          // Active la gestion des couleurs
    scrollok(stdscr, TRUE); // Autorise le défilement (scroll) du texte
    mousemask(ALL_MOUSE_EVENTS | REPORT_MOUSE_POSITION, NULL); // Active la détection de la souris

    // --- INITIALISATION DES COULEURS ---

    // -- TERRAINS --
    init_pair(1, 39, COLOR_BLACK);   // Eau : Bleu azur clair
    init_pair(2, 118, COLOR_BLACK);  // Plaine : Vert clair pomme
    init_pair(3, 28, COLOR_BLACK);   // Forêt : Vert sombre sapin
    init_pair(4, 130, COLOR_BLACK);  // Montagne : Marron/Terre
    init_pair(5, 214, COLOR_BLACK);  // Désert : Jaune sable / Orange
    init_pair(6, 253, COLOR_BLACK);  // Toundra : Blanc cassé glacé

    // -- UNITES JOUEUR --
    init_pair(10, 51, COLOR_BLACK);  // Guerrier : Cyan éclatant
    init_pair(11, 199, COLOR_BLACK); // Archer : Rose / Magenta
    init_pair(12, 129, COLOR_BLACK); // Cavalier : Violet
    init_pair(13, 226, COLOR_BLACK); // Colon : Jaune vif poussin
    init_pair(14, 202, COLOR_BLACK); // Unité générique (U) : Orange vif

    // -- ENNEMIS ET STRUCTURES --
    init_pair(20, 196, COLOR_BLACK); // Barbare : Rouge sang
    init_pair(21, 124, COLOR_BLACK); // Camp actif : Rouge bordeaux sombre
    init_pair(22, 240, COLOR_BLACK); // Camp détruit : Gris moyen
    
    // -- VILLES --
    init_pair(30, 15, COLOR_BLACK);  // Ville normale : Blanc pur
    init_pair(31, 15, 238);          // Ville muraille : Blanc pur sur fond Gris

    // -- BROUILLARD --
    init_pair(40, 236, COLOR_BLACK); // Zone Explorée : Gris extrêmement sombre

    // MENU PRINCIPAL
    int state = run_main_menu(&diff_level_ui, &enable_fow_ui);
    if (state == 0) {
        endwin();
        return 0;
    }

    if (diff_level_ui == 0) difficulty_from_string("facile", &difficulty);
    else if (diff_level_ui == 1) difficulty_from_string("normal", &difficulty);
    else difficulty_from_string("difficile", &difficulty);
    
    // Initialisation de la configuration de jeu (difficulty presets)
    GameConfig cfg;
    game_config_init_default(&cfg);
    game_config_apply_preset(&cfg, difficulty);
    if (has_production_override) {
        cfg.production_multiplier = production_multiplier_override;
    }
    if (has_growth_override) {
        cfg.growth_threshold_multiplier = growth_multiplier_override;
    }
    if (has_barbarian_spawn_override) {
        cfg.barbarian_spawn_multiplier = barbarian_spawn_multiplier_override;
    }
    if (has_famine_override) {
        cfg.famine_enabled = famine_override;
    }
    game_config_set_active(&cfg);


    // DEBUT DU JEU

    GameState game;
    memset(&game, 0, sizeof(GameState)); // Sécurité : on initialise tout à 0 pour éviter les valeurs indéterminées.
    
    clear();
    if (state == 1) {
        init_game(&game, width, height, seed);
        printw("[CIV] Nouvelle partie; Generation avec la graine : %d\n", seed);
    } else if(state == 2) {
        if (!load_game("savegame.sav", &game)) {
            printw("Échec du chargement de la partie. Démarrage d'une nouvelle partie.\n");
            init_game(&game, width, height, seed);
            printw("[CIV] Nouvelle partie; Generation avec la graine : %d\n", seed);
        }

    }

    printw("[CIV] Boucle principale lancée (prototype).\n");
    printw("\n>> Appuyez sur une touche pour commencer la partie...\n");
    refresh();
    getch();

    int city_id_counter=1;
    int selected_unit_idx = -1;
    int selected_city_idx = -1;
    char status_msg[128] = "Bienvenue ! Cliquez sur vos unites ou villes.";

    // Boucle centrale demandée dans le ticket.
    while (!game.game_over) {
        clear();
        int max_y, max_x;
        getmaxyx(stdscr, max_y, max_x);

        // On calcule ce que le joueur a le droit de voir(Brouillard de Guerre)
        update_fog_of_war(&game, enable_fow_ui);
        
        move(0, 0);
        display_map_color(&game.map);

        // 1. Affichage d'état (hook externe ou console)
        print_state_maxence_stub(&game);

        // --- DASHBOARD : ANCRÉ À DROITE DE L'ÉCRAN ---
        int ui_x = max_x - 35; // Le panneau de droite fera toujours 35 colonnes
        int log_y = max_y - 6; // Le panneau de logs prendra les 6 dernières lignes

        // Séparateurs visuels
        attron(COLOR_PAIR(30));
        for(int i=0; i<max_y; i++) mvaddch(i, ui_x - 1, ACS_VLINE); 
        mvhline(log_y - 1, 0, ACS_HLINE, max_x);
        attroff(COLOR_PAIR(30));

        attron(COLOR_PAIR(3) | A_BOLD);
        mvprintw(0, ui_x, "=== L'EMPIRE ===");
        attroff(COLOR_PAIR(3) | A_BOLD);
        mvprintw(2, ui_x, "Tour    : %d / %d", game.turn, nb_turns);
        mvprintw(3, ui_x, "Or      : %d", game.player.gold);
        mvprintw(4, ui_x, "Science : %d", game.player.science);

        mvprintw(6, ui_x, "-- EFFECTIFS --");
        mvprintw(7, ui_x, "Unites actives : %d / 8", game.player_unit_count);
        mvprintw(8, ui_x, "Villes         : %d", game.city_count);

        mvprintw(11, ui_x, "-- SELECTION --");
        
        if (selected_unit_idx != -1 && game.player_units[selected_unit_idx] && game.player_units[selected_unit_idx]->hp > 0) {
            Unit *u = game.player_units[selected_unit_idx];
            attron(COLOR_PAIR(10) | A_BOLD);
            mvprintw(13, ui_x, "Unite  : %s", unit_label(u->type));
            mvprintw(14, ui_x, "Pos    : (%d, %d)", u->x, u->y);
            mvprintw(15, ui_x, "PM     : %d / %d", u->pm, u->pm_max);
            mvprintw(16, ui_x, "PV     : %d / %d", u->hp, u->hp_max);
            mvprintw(18, ui_x, "[z/q/s/d] Deplacer");
            if (u->type == UNIT_SETTLER) mvprintw(19, ui_x, "[f] Fonder Ville");
            attroff(COLOR_PAIR(10) | A_BOLD);
        } else if (selected_city_idx != -1 && game.cities[selected_city_idx] && game.cities[selected_city_idx]->hp > 0) {
            City *c = game.cities[selected_city_idx];
            attron(COLOR_PAIR(30) | A_BOLD);
            mvprintw(13, ui_x, "Ville  : Capitale (%d, %d)", c->x, c->y);
            mvprintw(14, ui_x, "Pop    : %d", c->population);
            mvprintw(15, ui_x, "PV     : %d", c->hp);
            mvprintw(17, ui_x, "[v] Gerer Ville");
            attroff(COLOR_PAIR(30) | A_BOLD);
        } else {
            mvprintw(13, ui_x, "Aucune selection.");
        }

        // Ajout de la Seed en bas du carré de Dashboard
        attron(COLOR_PAIR(40));
        mvprintw(log_y - 2, ui_x + 2, "Seed : %d", seed);
        attroff(COLOR_PAIR(40));

        mvprintw(log_y, 0, "COMMANDES : [n] Finir Tour | [u] Choisir une unité | [c] Gerer Ville | [t] Technologies | [S] Sauvegarder (Maj+s) | [0] Voir les objectifs | [x] Quitter");
        attron(COLOR_PAIR(3) | A_BOLD);
        mvprintw(log_y + 2, 0, ">> %s", status_msg);
        attroff(COLOR_PAIR(3) | A_BOLD);
        refresh();
        
        int choice;
        choice = getch();
        strcpy(status_msg, ""); // On vide le message pour le prochain tour

        switch (choice) {

            case 'u':
            case 'U': {

                // Parcourt des unités pour trouver la suivante 
                int start_idx = (selected_unit_idx == -1) ? 0 : selected_unit_idx + 1;
                int found = 0;

                for (int i = 0; i < game.player_unit_count; i++) {
                    // On utilise le modulo pour boucler à partir du début si on atteint la fin de la liste
                    int idx = (start_idx + i) % game.player_unit_count;
                    if (game.player_units[idx] && game.player_units[idx]->hp > 0) {
                        selected_unit_idx = idx;
                        selected_city_idx = -1; // Désélectionne la ville
                        strcpy(status_msg, "Unite selectionnee(Clavier) !");
                        found = 1;
                        break;  
                    }
                }
                if (!found) {
                    strcpy(status_msg, "Aucune unite active disponible !");
                }
                break;
            }
            case 'c':
            case 'C': {

                
                // Parcourt des villes pour trouver la suivante
                int start_idx = (selected_city_idx == -1) ? 0 : selected_city_idx + 1;
                int found = 0;

                for (int i = 0; i < game.city_count; i++) {
                    int idx = (start_idx + i) % game.city_count;
                    if (game.cities[idx] && game.cities[idx]->hp > 0) {
                        selected_city_idx = idx;
                        selected_unit_idx = -1; // Désélectionne l'unité
                        strcpy(status_msg, "Ville selectionnee(Clavier) !");
                        found = 1;
                        break;  
                    }
                }
                if (!found) {
                    strcpy(status_msg, "Aucune ville disponible !");
                }
                break;
            }

            case 'b': // 'b' pour Back / Annuler
            case 'B':
            case 27:  // Code ASCII de la touche Echap (Escape)
                if (selected_unit_idx != -1 || selected_city_idx != -1) {
                    selected_unit_idx = -1;
                    selected_city_idx = -1;
                    strcpy(status_msg, "Selection annulee.");
                } else {
                    strcpy(status_msg, "Rien n'est selectionne.");
                }
                break;

            case 'o':
            case 'O': {
                clear(); // On nettoie l'écran
                int max_x;
                getmaxyx(stdscr, max_y, max_x);
            
                // Bordure de la fenêtre
                box(stdscr, 0, 0);
            
                // Titre principal
                attron(A_BOLD);
                mvprintw(2, (max_x - 30) / 2, "=== OBJECTIFS DE LA PARTIE ===");
                attroff(A_BOLD);
                
                int y = 5;
                
                // Section Victoire
                mvprintw(y++, 4, "--- CONDITIONS DE VICTOIRE ---");
                y++;
                mvprintw(y++, 6, "1. Victoire Scientifique :");
                mvprintw(y++, 9, "Debloquage de l'integralite des %d technologies disponibles.", MAX_TECHNOLOGIES);
                y++;
                mvprintw(y++, 6, "2. Victoire Territoriale :");
                mvprintw(y++, 9, "Possession d'au moins 10 villes simultanement pendant 5 tours consecutifs.");
                mvprintw(y++, 9, "(Statut actuel : %d / 5 tours avec 10 villes)", game.consecutive_turns_10_cities);
                
                y += 2;
                
                // Section Défaite
                mvprintw(y++, 4, "--- CONDITIONS DE DEFAITE ---");
                y++;
                mvprintw(y++, 6, "1. Destruction totale :");
                mvprintw(y++, 9, "destruction de toutes vos villes.");
                y++;
                mvprintw(y++, 6, "2. Fin du temps imparti :");
                mvprintw(y++, 9, "incapacité d'atteindre les objectifs dans les %d tours.", game.max_turns);
                
                y += 3;
                
                // Message de sortie centré
                attron(A_DIM);
                mvprintw(y, (max_x - 42) / 2, "[ Appuyez sur une touche pour revenir ]");
                attroff(A_DIM);
                
                refresh();
                getch(); // Attend que le joueur appuie sur une touche
                
                break; // Retourne au jeu (la boucle principale va redessiner la carte)
            }

            case KEY_MOUSE: {
                MEVENT event;
                if (getmouse(&event) == OK && (event.bstate & BUTTON1_CLICKED)) {
                    if (event.x < ui_x - 1 && event.y < log_y - 1) {
                        int found = 0;
                        int map_x = event.x / 2; // Division par 2 pour cibler la bonne case (caractères doubles)
                        int map_y = event.y;

                        for (int i = 0; i < game.player_unit_count; i++) {
                            if (game.player_units[i] && game.player_units[i]->hp > 0 && 
                                game.player_units[i]->x == map_x && game.player_units[i]->y == map_y) {
                                selected_unit_idx = i;
                                selected_city_idx = -1;
                                strcpy(status_msg, "Unite selectionnee.");
                                found = 1; break;
                            }
                        }
                        if (!found) {
                            for (int i = 0; i < game.city_count; i++) {
                                if (game.cities[i] && game.cities[i]->hp > 0 && 
                                    game.cities[i]->x == map_x && game.cities[i]->y == map_y) {
                                    selected_city_idx = i;
                                    selected_unit_idx = -1;
                                    strcpy(status_msg, "Ville selectionnee.");
                                    found = 1; break;
                                }
                            }
                        }
                        if (!found) { selected_unit_idx = -1; selected_city_idx = -1; }
                    }
                }
                continue;
            }
            case 'x':
            case 'X':
                game.game_over = 3; // Code de fin pour abandon
                continue;

            case 'f':
            case 'F': {
                if (selected_unit_idx != -1 && game.player_units[selected_unit_idx]) {
                Unit *mon_unite_f = game.player_units[selected_unit_idx];

                // 1. Sécurité : l'unité existe et est en vie
                if (mon_unite_f == NULL || mon_unite_f->hp <= 0) {
                    strcpy(status_msg, "Cette unite est morte ou n'existe pas.");
                    break; // On utilise break au lieu de continue
                }
                
                // 2. Est-ce bien un Colon ?
                if (mon_unite_f->type != UNIT_SETTLER) {
                    strcpy(status_msg, "Erreur : Seul un Colon peut fonder une ville.");
                    break;
                }

                // 3. Le Colon doit avoir des PM pour agir
                if (mon_unite_f->pm <= 0) {
                    strcpy(status_msg, "Ce Colon n'a plus de PM. Attendez le tour suivant.");
                    break;
                }

                // 4. Tentative de fondation via la fonction de Wassim
                if (found_city(mon_unite_f, &game.map, &city_id_counter, game.cities, &game.city_count)) {
                    
                    // --- SUCCÈS ---
                    strcpy(status_msg, "Ville fondee avec succes !");
                    
                    // A) On retire le Colon de la carte (TRÈS IMPORTANT)
                    game.map.tiles[mon_unite_f->y][mon_unite_f->x].unit = NULL;
                    
                    // B) On "tue" le Colon dans le tableau du joueur
                    mon_unite_f->hp = 0; 
                    game.player_units[selected_unit_idx] = NULL; 
                    
                    // C) On désélectionne l'unité morte
                    selected_unit_idx = -1;
                    
                } else {
                    // --- ÉCHEC ---
                    // Comme on ne voit pas les printw de Wassim, on met un message générique clair
                    strcpy(status_msg, "Impossible : Eau, Limite atteinte ou Ville a moins de 4 cases !");
                }
            } else {
                strcpy(status_msg, "Selectionnez un Colon d'abord !");
            }
            break; // On sort du switch pour redessiner l'écran
            }

            case 'v':
            case 'V':
                if (game.city_count == 0) {
                    strcpy(status_msg, "Vous ne possedez aucune ville !");
                    continue;
                }

                if (selected_city_idx != -1 && game.cities[selected_city_idx]) {
                    // Sécurité : on vérifie que l'index est valide et que la ville existe
                    if (game.cities[selected_city_idx]->hp <= 0) {
                        strcpy(status_msg, "Index de ville invalide.");
                        continue;
                    }
                    // On envoie la ville sélectionnée à la fonction de gestion
                    control_city_project(game.cities[selected_city_idx]);
                } else {
                    strcpy(status_msg, "Selectionnez une ville avec la souris d'abord !");
                }
                continue;
                
            case 't':
            case 'T': {
                clear(); 
                int max_x;
                getmaxyx(stdscr, max_y, max_x);
                
                // Dessin d'une bordure
                attron(COLOR_PAIR(1));
                box(stdscr, 0, 0);
                attroff(COLOR_PAIR(1));

                // Titre
                attron(A_BOLD | COLOR_PAIR(3));
                mvprintw(2, (max_x - 25) / 2, "=== ARBRE TECHNOLOGIQUE ===");
                attroff(A_BOLD | COLOR_PAIR(3));

                mvprintw(4, 5, "Cagnotte de Science : %d", game.player.science);

                // Affichage de la recherche actuelle
                if (game.player.current_research != TECH_NONE) {
                    mvprintw(5, 5, "Recherche en cours : [%s] (%d / %d Science)", 
                             TECH_CATALOG[game.player.current_research].name,
                             game.player.science,
                             TECH_CATALOG[game.player.current_research].cost);
                } else {
                    mvprintw(5, 5, "Aucune recherche en cours.");
                }

                mvprintw(7, 5, "Technologies disponibles à la recherche :");
                
                int row = 9;
                int options_dispo = 0;
                
                // Parcours du catalogue
                for (int i = 0; i < MAX_TECHNOLOGIES; i++) {
                    if (game.player.unlocked_techs[i]) {
                        // Déjà débloqué (affichage grisé ou simple)
                        mvprintw(row++, 5, "[X] %s (Acquis)", TECH_CATALOG[i].name);
                    } else if (can_research(&game.player, i)) {
                        // Disponible à la recherche : on ajoute la description !
                        const char* desc = "";
                        switch(i) {
                            case TECH_CHASSE: desc = "+1 Nourriture sur les tuiles Forêt"; break;
                            case TECH_AGRICULTURE: desc = "+10%% Nourriture dans toutes les villes"; break;
                            case TECH_ARTISANAT: desc = "Débloque Guerrier et Archer, +10%% Production"; break;
                            case TECH_ECRITURE: desc = "Débloque Bibliothèque, +10%% Science"; break;
                            case TECH_EQUITATION: desc = "Débloque Cavalier, +1 PM (unités terrestres)"; break;
                            case TECH_IRRIGATION: desc = "+20%% Nourriture supplémentaire"; break;
                            case TECH_MAÇONNERIE: desc = "Débloque Muraille (Double PV de la ville)"; break;
                            case TECH_COMMERCE: desc = "Débloque Marché, +10%% Or"; break;
                            case TECH_BATEAU: desc = "Permet de traverser l'Eau (Coût: 2 PM)"; break;
                        }
                        // On affiche le numéro pour que le joueur puisse choisir
                        attron(A_BOLD);
                        mvprintw(row++, 5, "[%d] %s (Coût: %d) -> %s", i, TECH_CATALOG[i].name, TECH_CATALOG[i].cost, desc);
                        attroff(A_BOLD);
                        options_dispo++;
                    }
                }

                if (options_dispo == 0) {
                    mvprintw(row++, 5, "Aucune nouvelle technologie disponible (vérifiez les prérequis).");
                }

                mvprintw(row + 2, 5, "Entrez le numéro de la technologie à chercher, ou 'x' pour annuler : ");
                refresh();

                // Gestion du choix du joueur
                int tech_choice = getch();
                
                if (tech_choice >= '0' && tech_choice <= '8') {
                    int tech_idx = tech_choice - '0';
                    if (can_research(&game.player, tech_idx)) {
                        start_research(&game.player, tech_idx);
                        sprintf(status_msg, "\033[1;33mNouvelle recherche lancee : %s\033[0m", TECH_CATALOG[tech_idx].name);
                    } else {
                        strcpy(status_msg, "Recherche impossible (prérequis manquants).");
                    }
                } else if (tech_choice == 'x' || tech_choice == 'X') {
                    strcpy(status_msg, "Menu technologique ferme.");
                } else {
                    strcpy(status_msg, "Touche invalide.");
                }
                
                // On met un break pour sortir du case et laisser la boucle while redessiner la carte
                break;   
            }
          
          

            case 'S':
                save_game("savegame.sav", &game);
                strcpy(status_msg, "Partie sauvegardee avec succes !");
                continue;

            case '3':
            case 'n': {

                TechType tech_en_cours = game.player.current_research;
                
                // 5. Le joueur a fini de jouer, l'ordinateur (Barbares d'Armel) joue son tour
                next_turn(&game);
                selected_unit_idx = -1; // Deselection de l'unité à la fin du tour

                selected_unit_idx = -1; // On désélectionne l'unité à la fin du tour

                if (tech_en_cours != TECH_NONE && game.player.current_research == TECH_NONE) {
                    sprintf(status_msg, "EUREKA ! Technologie [%s] decouverte !", TECH_CATALOG[tech_en_cours].name);
                } else {
                    sprintf(status_msg, "Tour %d termine. A vous de jouer !", game.turn - 1);
                }
                
                check_victory_and_defeat(&game);
                continue;
            }

            case 'z': case 'q': case 's': case 'd':
                if (selected_unit_idx == -1 || !game.player_units[selected_unit_idx]) {
                    strcpy(status_msg, "Erreur : Selectionnez d'abord une unite avec la souris.");
                    continue;
                }

                Unit *mon_unite = game.player_units[selected_unit_idx];

                // Sécurité : on vérifie que l'unité existe et est en vie
                if (mon_unite == NULL || mon_unite->hp <= 0) {
                    strcpy(status_msg, "Cette unite est morte ou n'existe pas.");
                    continue;
                }

                if (mon_unite->pm <= 0) {
                    strcpy(status_msg, "Cette unite n'a plus de PM. Choisissez-en une autre.");
                    continue;
                }

                // 3. Calcul des futures coordonnées selon la touche pressée
                int target_x = mon_unite->x;
                int target_y = mon_unite->y;

                if (choice == 'z') target_y -= 1;   //haut
                else if (choice == 's') target_y += 1;  //Bas
                else if (choice == 'q') target_x -= 1;   // Gauche
                else if (choice == 'd') target_x += 1;  // Droite

                // 4. Logique de dépacement et application de ma physique (can_move, resolve_combat, etc.)

                // Vérification des coordonnées de la carte pour éviter des erreurs d'accès mémoire
                if (target_x >= 0 && target_x < game.map.width && target_y >= 0 && target_y < game.map.height) {

                    // On cible la tuile d'arrivée 
                    Tile *target_tile = &game.map.tiles[target_y][target_x];

                    // Appel de la physique du jeu
                    if (can_move(mon_unite, target_tile, &game.player)) {

                        move(log_y + 4, 0); // On place le curseur en bas pour les messages de combat
                        int has_attacked = 0; // Permet de savoir si on a donné un coup pour détruire le PM
                        bool action_logged = false;

                        // A1. Combat contre une unité ennemie (Barbare)
                        if (target_tile->unit && target_tile->unit->owner != mon_unite->owner) {
                            mon_unite->pm--; // On consomme le PM pour attaquer
                            has_attacked = 1;
                            action_logged = true;
                            printw("\n Combat engagé contre le barbare ! \n");
                            printw("-> Avant : Toi (%d PV) vs Barbare (%d PV)\n", mon_unite->hp, target_tile->unit->hp);
                            resolve_combat(mon_unite, target_tile->unit);
                            printw("-> Apres : Toi (%d PV) vs Barbare (%d PV)\n", mon_unite->hp, target_tile->unit->hp);

                            // VERDICT DU COMBAT

                            if(target_tile->unit->hp <= 0) {
                                printw("VICTOIRE ! Le barbare est mort !\n");
                                target_tile->unit = NULL; //On vide la place de parking du Barbare
                            }
                        }

                        // A2. Combat contre une ville ennemie (Barbare)
                        else if (target_tile->city && target_tile->city->owner != mon_unite->owner) {
                            mon_unite->pm--;
                            has_attacked = 1;
                            action_logged = true;
                            printw("\n Attaque de la ville ennemie ! \n");
                            printw("-> Avant : Toi (%d PV) vs Ville (%d PV)\n", mon_unite->hp, target_tile->city->hp);
                            resolve_combat_city(mon_unite, target_tile->city);
                            printw("-> Apres : Toi (%d PV) vs Ville (%d PV)\n", mon_unite->hp, target_tile->city->hp);

                            if (target_tile->city->hp <= 0) {
                                printw("VICTOIRE ! La ville a été rasée !\n");
                                free(target_tile->city);
                                target_tile->city = NULL; //On vide la place de parking de la ville
                            }
                        }

                        // A3. Pillage d'un camp barbare
                        else if (target_tile->camp != NULL && target_tile->camp->destroyed == 0) {
                            action_logged = true;
                            printw("\n Vous avez détruit un campement barbare ! \n");
                            target_tile->camp->destroyed = 1; // On détruit le camp(pas de combat)
                        }

                        // VERDICT DU MOUVEMENT APRÈS LES COMBATS




                        if (mon_unite->hp <= 0) {
                            action_logged = true;
                            printw(" DEFAITE ! Votre unité a été massacre ... \n");
                            game.map.tiles[mon_unite->y][mon_unite->x].unit = NULL; // on efface notre joueur
                            selected_unit_idx = -1;
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

                            strcpy(status_msg, "Deplacement reussi !");
                        }

                        // C. L'obstacle est toujours lè(l'ennemi a surcécu, le mouvement est annulé)
                        else if (has_attacked) {
                            mon_unite->pm--; // Même si on a attaqué, le mouvement est annulé mais on perd quand même le PM
                            action_logged = true;
                            printw("\n La cible a survécu à votre attaque, Déplacement annulé mais PM consommé ! \n");
                        }

                        // Pause uniquement s'il y a eu un combat
                        if (action_logged) {
                            attron(COLOR_PAIR(3));
                            printw("\n[Appuyez sur une touche pour continuer...]");
                            attroff(COLOR_PAIR(3));
                            refresh(); getch();
                        }

                    } else {
                        strcpy(status_msg, "Déplacement impossible (pas assez de PM ou terrain bloqué) !");
                    }
                } else {
                    strcpy(status_msg, " Tu essaies de sortir de la carte.");
                }
                continue;
        }
    }
    
    printw("\nFin de partie (prototype).\n");

    refresh();
    getch();

    cleanup_game(&game);
    endwin();
    // 2. Gestion de la fin de partie 

    if (game.game_over == 1 || game.game_over == 2) {
        int final_score = calculate_final_score(&game);

        printf("\n=====================================================\n");
        if (game.game_over == 1) {
            printf("\033[1;33m                VICTOIRE !                       \033[0m\n");
        } else {
            printf("\033[1;31m                DEFAITE !                       \033[0m\n");
        }
        printf("======================================================\n");
        printf("Score final     : %d points\n", final_score);
        printf("Survie          : %d / %d tours\n", game.turn, nb_turns);
        printf("--------------------------------------------------\n");

        char player_name[50];
        printf("Entrez votre nom pour enregistrer votre score : ");
        if (scanf("%49s", player_name) == 1) {
            // Enregistrer le score
            save_final_score(player_name, final_score, game.turn, game.game_over);
            printf("Score enregistré avec succés dans scores.txt !\n");

        }
    } else {
        printf("Partie terminée prématurément. Aucun score enregistré.\n");
    }

    cleanup_game(&game);



    return 0;
}
