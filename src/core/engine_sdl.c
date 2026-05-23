#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <string.h>
#include <SDL2/SDL.h>

#include "core/engine_sdl.h"
#include "core/game.h"
#include "core/map.h"
#include "core/types.h"

// Fonction locale utilitaire pour dessiner les couleurs de terrain en arrière-plan
static void draw_terrain_primitive(SDL_Renderer *renderer, TerrainType type, SDL_Rect *rect) {
    switch (type) {
        case TERRAIN_EAU:      SDL_SetRenderDrawColor(renderer, 0, 102, 204, 255); break;  // Bleu
        case TERRAIN_PLAINE:   SDL_SetRenderDrawColor(renderer, 51, 153, 51, 255); break;  // Vert clair
        case TERRAIN_FORET:    SDL_SetRenderDrawColor(renderer, 0, 102, 0, 255); break;    // Vert sapin
        case TERRAIN_MONTAGNE: SDL_SetRenderDrawColor(renderer, 128, 128, 128, 255); break; // Gris roche
        case TERRAIN_DESERT:   SDL_SetRenderDrawColor(renderer, 230, 184, 0, 255); break;   // Jaune sable
        case TERRAIN_TOUNDRA:  SDL_SetRenderDrawColor(renderer, 220, 220, 240, 255); break; // Blanc neige
        default:               SDL_SetRenderDrawColor(renderer, 50, 50, 50, 255); break;
    }
    SDL_RenderFillRect(renderer, rect);
}

// Fonction utilitaire pour bloquer la caméra dans les limites
static void clamp_camera(int *x, int *y, int map_w, int map_h, int screen_w, int screen_h) {
    int max_x = (map_w * TILE_SIZE) - screen_w;
    int max_y = (map_h * TILE_SIZE) - screen_h;

    if (*x < 0) *x = 0;
    if (max_x > 0 && *x > max_x) *x = max_x;
    else if (max_x <= 0) *x = 0;

    if (*y < 0) *y = 0;
    if (max_y > 0 && *y > max_y) *y = max_y;
    else if (max_y <= 0) *y = 0;
}

void run_sdl_mode(GameState *game, int nb_turns, bool enable_fow_ui) {
    (void)nb_turns; // Évite les avertissements de compilation non utilisée

    // --- TICKET 1 : INITIALISATION DE LA SDL ---
    if (SDL_Init(SDL_INIT_VIDEO) != 0) {
        fprintf(stderr, "[SDL2 ERROR] Impossible d'initialiser la vidéo : %s\n", SDL_GetError());
        return;
    }

    // On récupère les infos de l'écran du user
    SDL_DisplayMode dm;
    SDL_GetCurrentDisplayMode(0, &dm);
    
    int screen_w = dm.w;
    int screen_h = dm.h;

    // Créer la fenêtre en plein écran fenêtré (Borderless)
    SDL_Window *window = SDL_CreateWindow("Civilization - Edition Graphique",
                                          SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED,
                                          screen_w, screen_h, SDL_WINDOW_SHOWN | 
                                          SDL_WINDOW_MAXIMIZED | SDL_WINDOW_FULLSCREEN_DESKTOP);

    // Variables de la cam (coordonnées en pixels du coin haut-gauche comme en cli)
    int camera_x = 0;
    int camera_y = 0;
    int camera_speed = 32; // Vitesse de déplacement de la cam

    if (!window) {
        fprintf(stderr, "[SDL2 ERROR] Erreur création de fenêtre : %s\n", SDL_GetError());
        SDL_Quit();
        return;
    }

    SDL_Renderer *renderer = SDL_CreateRenderer(window, -1, SDL_RENDERER_ACCELERATED | SDL_RENDERER_PRESENTVSYNC);
    if (!renderer) {
        fprintf(stderr, "[SDL2 ERROR] Erreur création du Renderer : %s\n", SDL_GetError());
        SDL_DestroyWindow(window);
        SDL_Quit();
        return;
    }

    // Configuration essentielle pour appliquer le masque de transparence du brouillard
    SDL_SetRenderDrawBlendMode(renderer, SDL_BLENDMODE_BLEND);

    printf("\n=======================================================\n");
    printf("[SDL2 MOTEUR] Fenêtre graphique initialisée avec succès !\n");
    printf("-> Utilisez votre Souris dans la fenêtre pour sélectionner.\n");
    printf("-> Regardez votre Terminal pour lire les informations d'état.\n");
    printf("=======================================================\n");

    int sel_unit_idx = -1;
    int sel_city_idx = -1;
    char console_status[128] = "Mode Graphique actif. Cliquez sur une entité.";

    // --- TICKET 1 : LA BOUCLE CENTRALE GRAPHIC ---
    SDL_Event event;
    while (!game->game_over) {

        // A. TRAITEMENT DES INPUTS ET DES ÉVÉNEMENTS
        while (SDL_PollEvent(&event)) {
            if (event.type == SDL_QUIT) {
                game->game_over = 3; // Demande de fermeture de l'application (croix rouge)
            }
            
            // Écoute du clavier
            else if (event.type == SDL_KEYDOWN) {
                switch (event.key.keysym.sym) {
                    case SDLK_ESCAPE:
                        game->game_over = 3; // Quitter le jeu avec Echap
                        break;
                    case SDLK_n: // Passer le tour
                        next_turn(game);
                        sel_unit_idx = -1;
                        sprintf(console_status, "Tour %d terminé. À vous de jouer !", game->turn - 1);
                        break;
                    
                    // MOUVEMENTS DE LA CAMERA
                    case SDLK_UP:    camera_y -= camera_speed; break;
                    case SDLK_DOWN:  camera_y += camera_speed; break;
                    case SDLK_LEFT:  camera_x -= camera_speed; break;
                    case SDLK_RIGHT: camera_x += camera_speed; break;
                    
                    case SDLK_F11: { //F11 pour basculer en plein écran
                        Uint32 fullScreenFlag = SDL_WINDOW_FULLSCREEN_DESKTOP;
                        bool isFullScreen = SDL_GetWindowFlags(window) & fullScreenFlag;

                        // Si on est plein écran, on repasse en fenêtré, sinon l'inverse
                        SDL_SetWindowFullscreen(window, isFullScreen ? 0 : fullScreenFlag);
                        SDL_GetWindowSize(window, &screen_w, &screen_h); // MAJ des dimensions
                        break;
                    }
                    default:
                        break;
                }
                // On applique les limites via la fonction utilitaire
                clamp_camera(&camera_x, &camera_y, game->map.width, game->map.height, screen_w, screen_h);
            }

            // Écoute de la souris (Sélection de tes éléments graphiques)
            else if (event.type == SDL_MOUSEBUTTONDOWN) {
                if (event.button.button == SDL_BUTTON_LEFT) {
                    // Traduction magique des coordonnées pixel en index de matrice C
                    int click_grid_x = (event.button.x + camera_x) / TILE_SIZE;
                    int click_grid_y = (event.button.y + camera_y) / TILE_SIZE;

                    if (click_grid_x >= 0 && click_grid_x < game->map.width &&
                        click_grid_y >= 0 && click_grid_y < game->map.height) {
                        
                        bool target_found = false;

                        // Vérification de la présence d'une unité
                        for (int i = 0; i < game->player_unit_count; i++) {
                            if (game->player_units[i] && game->player_units[i]->hp > 0 &&
                                game->player_units[i]->x == click_grid_x && game->player_units[i]->y == click_grid_y) {
                                sel_unit_idx = i;
                                sel_city_idx = -1;
                                sprintf(console_status, "Unité sélectionnée : Position (%d, %d)", click_grid_x, click_grid_y);
                                target_found = true;
                                break;
                            }
                        }

                        // Sinon, vérification d'une ville
                        if (!target_found) {
                            for (int i = 0; i < game->city_count; i++) {
                                if (game->cities[i] && game->cities[i]->hp > 0 &&
                                    game->cities[i]->x == click_grid_x && game->cities[i]->y == click_grid_y) {
                                    sel_city_idx = i;
                                    sel_unit_idx = -1;
                                    sprintf(console_status, "Ville sélectionnée : Position (%d, %d)", click_grid_x, click_grid_y);
                                    target_found = true;
                                    break;
                                }
                            }
                        }

                        if (!target_found) {
                            sel_unit_idx = -1;
                            sel_city_idx = -1;
                            strcpy(console_status, "Clic sur une case vide.");
                        }

                        // Affichage synchrone des données dans le terminal d'exécution
                        printf("\n[UI STATUS] >> %s | Or: %d | Science: %d\n", 
                               console_status, game->player.gold, game->player.science);
                    }
                }
            }
        }

        // B. RENDU DES GRAPHIQUES (Ticket 3)
        SDL_SetRenderDrawColor(renderer, 0, 0, 0, 255);
        SDL_RenderClear(renderer);

        update_fog_of_war(game, enable_fow_ui);

        // Double boucle imbriquée pour balayer et dessiner chaque tuile
        for (int y = 0; y < game->map.height; y++) {
            for (int x = 0; x < game->map.width; x++) {
                Tile *tile = &game->map.tiles[y][x];
                
                // 1. Calcul de la position à l'écran en soustrayant la caméra
                int screen_pos_x = (x * TILE_SIZE) - camera_x;
                int screen_pos_y = (y * TILE_SIZE) - camera_y;

                // si la tuile est en dehors de la caméra on ne l'affiche pas
                if (screen_pos_x + TILE_SIZE < 0 || screen_pos_x > screen_w ||
                    screen_pos_y + TILE_SIZE < 0 || screen_pos_y > screen_h) {
                    continue; 
                }
                
                // Le rectangle principal de la tuile
                SDL_Rect visual_rect = { screen_pos_x, screen_pos_y, TILE_SIZE, TILE_SIZE };

                // Dessin du terrain de la tuile
                draw_terrain_primitive(renderer, tile->type, &visual_rect);

                // Rendu des entités actives sur la tuile
                if (tile->visibility != FOG_HIDDEN) {
                    
                    // Rendu de la ville
                    if (tile->city && tile->city->hp > 0) {
                        SDL_Rect city_box = { screen_pos_x + 4, screen_pos_y + 4, TILE_SIZE - 8, TILE_SIZE - 8 };
                        SDL_SetRenderDrawColor(renderer, 245, 245, 245, 255); 
                        SDL_RenderFillRect(renderer, &city_box);
                    }

                    // Rendu des unités
                    if (tile->unit && tile->unit->hp > 0) {
                        SDL_Rect unit_box = { screen_pos_x + 9, screen_pos_y + 9, TILE_SIZE - 18, TILE_SIZE - 18 };
                        if (tile->unit->owner == &game->player) {
                            SDL_SetRenderDrawColor(renderer, 0, 255, 255, 255); // Cyan (Joueur)
                        } else {
                            SDL_SetRenderDrawColor(renderer, 255, 0, 0, 255);   // Rouge (Barbare)
                        }
                        SDL_RenderFillRect(renderer, &unit_box);
                    }

                    // Rendu d'un campement barbare
                    if (tile->camp && tile->camp->destroyed == 0) {
                        SDL_Rect camp_box = { screen_pos_x + 6, screen_pos_y + 6, TILE_SIZE - 12, TILE_SIZE - 12 };
                        SDL_SetRenderDrawColor(renderer, 150, 0, 0, 255);
                        SDL_RenderDrawRect(renderer, &camp_box);
                    }
                }

                // Brouillard de Guerre
                if (tile->visibility == FOG_HIDDEN) {
                    SDL_SetRenderDrawColor(renderer, 0, 0, 0, 255);
                    SDL_RenderFillRect(renderer, &visual_rect);
                } 
                else if (tile->visibility == FOG_EXPLORED) {
                    SDL_SetRenderDrawColor(renderer, 0, 0, 0, 160);
                    SDL_RenderFillRect(renderer, &visual_rect);
                }

                // Surlignage de sélection
                if ((sel_unit_idx != -1 && game->player_units[sel_unit_idx] && game->player_units[sel_unit_idx]->x == x && game->player_units[sel_unit_idx]->y == y) ||
                    (sel_city_idx != -1 && game->cities[sel_city_idx] && game->cities[sel_city_idx]->x == x && game->cities[sel_city_idx]->y == y)) {
                    SDL_SetRenderDrawColor(renderer, 255, 255, 0, 255);
                    SDL_RenderDrawRect(renderer, &visual_rect);
                }
            }
        }

        // Commutation immédiate des toiles graphiques (Double buffering)
        SDL_RenderPresent(renderer);
    }

    // --- SÉCURITÉ ET LIQUIDATION DE LA MÉMOIRE EN FIN DE PARTIE ---
    SDL_DestroyRenderer(renderer);
    SDL_DestroyWindow(window);
    SDL_Quit();
    printf("[SDL MOTEUR] Session graphique libérée proprement.\n");
}