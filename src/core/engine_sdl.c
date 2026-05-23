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

void run_sdl_mode(GameState *game, int nb_turns, bool enable_fow_ui) {
    (void)nb_turns; // Évite les avertissements de compilation non utilisée

    // --- TICKET 1 : INITIALISATION DE LA SDL ---
    if (SDL_Init(SDL_INIT_VIDEO) != 0) {
        fprintf(stderr, "[SDL2 ERROR] Impossible d'initialiser la vidéo : %s\n", SDL_GetError());
        return;
    }

    int screen_w = game->map.width * TILE_SIZE;
    int screen_h = game->map.height * TILE_SIZE;

    SDL_Window *window = SDL_CreateWindow("Civilization - Edition Graphique",
                                          SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED,
                                          screen_w, screen_h, SDL_WINDOW_SHOWN);
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
                    default:
                        break;
                }
            }

            // Écoute de la souris (Sélection de tes éléments graphiques)
            else if (event.type == SDL_MOUSEBUTTONDOWN) {
                if (event.button.button == SDL_BUTTON_LEFT) {
                    // Traduction magique des coordonnées pixel en index de matrice C
                    int click_grid_x = event.button.x / TILE_SIZE;
                    int click_grid_y = event.button.y / TILE_SIZE;

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
                
                // Emplacement géométrique précis en pixels sur la fenêtre
                SDL_Rect visual_rect = { x * TILE_SIZE, y * TILE_SIZE, TILE_SIZE, TILE_SIZE };

                // 1. Dessin obligatoire du terrain sous-jacent
                draw_terrain_primitive(renderer, tile->type, &visual_rect);

                // 2. Rendu des entités actives si la tuile n'est pas plongée dans le noir absolu
                if (tile->visibility != FOG_HIDDEN) {
                    
                    // Rendu de la ville (Gros carré central blanc)
                    if (tile->city && tile->city->hp > 0) {
                        SDL_Rect city_box = { x * TILE_SIZE + 4, y * TILE_SIZE + 4, TILE_SIZE - 8, TILE_SIZE - 8 };
                        SDL_SetRenderDrawColor(renderer, 245, 245, 245, 255); 
                        SDL_RenderFillRect(renderer, &city_box);
                    }

                    // Rendu des unités (Petit carré concentré au milieu)
                    if (tile->unit && tile->unit->hp > 0) {
                        SDL_Rect unit_box = { x * TILE_SIZE + 9, y * TILE_SIZE + 9, TILE_SIZE - 18, TILE_SIZE - 18 };
                        
                        if (tile->unit->owner == &game->player) {
                            SDL_SetRenderDrawColor(renderer, 0, 255, 255, 255); // Cyan éclatant (Joueur)
                        } else {
                            SDL_SetRenderDrawColor(renderer, 255, 0, 0, 255);   // Rouge sang (Barbare)
                        }
                        SDL_RenderFillRect(renderer, &unit_box);
                    }

                    // Rendu d'un campement barbare actif (Carré évidé bordeaux)
                    if (tile->camp && tile->camp->destroyed == 0) {
                        SDL_Rect camp_box = { x * TILE_SIZE + 6, y * TILE_SIZE + 6, TILE_SIZE - 12, TILE_SIZE - 12 };
                        SDL_SetRenderDrawColor(renderer, 150, 0, 0, 255);
                        SDL_RenderDrawRect(renderer, &camp_box);
                    }
                }

                // 3. Application immédiate des filtres de Brouillard de Guerre
                if (tile->visibility == FOG_HIDDEN) {
                    SDL_SetRenderDrawColor(renderer, 0, 0, 0, 255); // Cache noir opaque
                    SDL_RenderFillRect(renderer, &visual_rect);
                } 
                else if (tile->visibility == FOG_EXPLORED) {
                    SDL_SetRenderDrawColor(renderer, 0, 0, 0, 160); // Filtre d'obscurité semi-transparent
                    SDL_RenderFillRect(renderer, &visual_rect);
                }

                // Surlignage visuel si l'élément sur cette case est sélectionné par l'utilisateur
                if ((sel_unit_idx != -1 && game->player_units[sel_unit_idx] && game->player_units[sel_unit_idx]->x == x && game->player_units[sel_unit_idx]->y == y) ||
                    (sel_city_idx != -1 && game->cities[sel_city_idx] && game->cities[sel_city_idx]->x == x && game->cities[sel_city_idx]->y == y)) {
                    SDL_SetRenderDrawColor(renderer, 255, 255, 0, 255); // Contour Jaune vif
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