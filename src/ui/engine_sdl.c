#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <string.h>
#include <time.h>
#include <SDL2/SDL.h>
#include <SDL2/SDL_ttf.h>
#include <SDL2/SDL_image.h>

#include "ui/engine_sdl.h"
#include "core/game.h"
#include "core/map.h"
#include "core/types.h"
#include "core/physics.h"
#include "core/tech.h"
#include "core/save.h" 
#include "core/wonder.h"
#include "core/special_ressources.h"
#include "core/ressources.h"

// --- LISTE DES MENUS ---
typedef enum {
    VIEW_MAIN_MENU,
    VIEW_OPTIONS_MENU,
    VIEW_RULES_MENU,
    VIEW_SCORE_HISTORY,
    VIEW_GAME,
    VIEW_TECH_MENU,
    VIEW_CITY_MENU,
    VIEW_GAME_OVER
} SDL_ActiveView;

// Fonction utilitaire pour le nom des unités
static const char *unit_label(UnitType type) {
    switch (type) {
        case UNIT_WARRIOR:  return "Guerrier";
        case UNIT_ARCHER:   return "Archer";
        case UNIT_CAVALIER: return "Cavalier";
        case UNIT_SETTLER:
        default:            return "Colon";
    }
}

// Fonction utilitaire pour récupérer le coût et le nom des projets classiques
static int get_project_cost(ProjectType p) {
    switch(p) {
        case PROJECT_SETTLER: return 50;
        case PROJECT_WARRIOR: return 40;
        case PROJECT_ARCHER: return 55;
        case PROJECT_CAVALIER: return 60;
        case PROJECT_GRANARY: return 30;
        case PROJECT_WORKSHOP: return 40;
        case PROJECT_LIBRARY: return 50;
        case PROJECT_MARKET: return 40;
        case PROJECT_BARRACKS: return 60;
        case PROJECT_WALL: return 80;
        default: return 0;
    }
}

static const char* get_project_name(ProjectType p) {
    switch(p) {
        case PROJECT_SETTLER: return "Colon";
        case PROJECT_WARRIOR: return "Guerrier";
        case PROJECT_ARCHER: return "Archer";
        case PROJECT_CAVALIER: return "Cavalier";
        case PROJECT_GRANARY: return "Grenier";
        case PROJECT_WORKSHOP: return "Atelier";
        case PROJECT_LIBRARY: return "Bibliotheque";
        case PROJECT_MARKET: return "Marche";
        case PROJECT_BARRACKS: return "Caserne";
        case PROJECT_WALL: return "Muraille";
        default: return "Inconnu";
    }
}

// Fonction pour découper et dessiner un morceau de spritesheet (16x16)
static void draw_texture_part(SDL_Renderer *renderer, SDL_Texture *tex, int src_x, int src_y, int dest_x, int dest_y) {
    if (!tex) return;
    SDL_Rect src = { src_x, src_y, 16, 16 };
    SDL_Rect dest = { dest_x, dest_y, TILE_SIZE, TILE_SIZE };
    SDL_RenderCopy(renderer, tex, &src, &dest);
}

// Fonction locale qui va permettre d'écrire du texte 
static void draw_text(SDL_Renderer *renderer, TTF_Font *font, const char *text, int x, int y, SDL_Color color) {
    if (!font || !text) return;
    SDL_Surface *surface = TTF_RenderText_Blended(font, text, color);
    if (!surface) return;
    SDL_Texture *texture = SDL_CreateTextureFromSurface(renderer, surface);
    SDL_Rect dest = { x, y, surface->w, surface->h };
    SDL_RenderCopy(renderer, texture, NULL, &dest);
    SDL_DestroyTexture(texture);
    SDL_FreeSurface(surface);
}

// Fonction locale pour centrer le texte 
static void draw_text_centered(SDL_Renderer *renderer, TTF_Font *font, const char *text, int center_x, int y, SDL_Color color) {
    if (!font || !text) return;
    SDL_Surface *surface = TTF_RenderText_Blended(font, text, color);
    if (!surface) return;
    SDL_Texture *texture = SDL_CreateTextureFromSurface(renderer, surface);
    SDL_Rect dest = { center_x - (surface->w / 2), y, surface->w, surface->h };
    SDL_RenderCopy(renderer, texture, NULL, &dest);
    SDL_DestroyTexture(texture);
    SDL_FreeSurface(surface);
}

// Fonction utilitaire pour bloquer la caméra (évite de "perdre" la map si on va trop loin)
static void clamp_camera(int *x, int *y, int map_w, int map_h, int screen_w, int screen_h) {
    int dashboard_width = 420;
    // La zone visible s'arrête avant le dashboard
    int max_x = (map_w * TILE_SIZE) - (screen_w - dashboard_width);
    int max_y = (map_h * TILE_SIZE) - screen_h;

    if (*x < 0) *x = 0;
    if (max_x > 0 && *x > max_x) *x = max_x; else if (max_x <= 0) *x = 0; 
    if (*y < 0) *y = 0;
    if (max_y > 0 && *y > max_y) *y = max_y; else if (max_y <= 0) *y = 0;
}

// Fonction utilitaire pour connecter SDL et la physique du jeu
static void attempt_move_sdl(GameState *game, int sel_unit_idx, int dx, int dy, char *console_status) {
    if (sel_unit_idx == -1 || !game->player_units[sel_unit_idx]) return;
    Unit *unit = game->player_units[sel_unit_idx];
    if (unit->hp <= 0) return; 

    int target_x = unit->x + dx;
    int target_y = unit->y + dy;

    if (target_x < 0 || target_x >= game->map.width || target_y < 0 || target_y >= game->map.height) {
        strcpy(console_status, "Deplacement impossible : Bord de carte.");
        return;
    }

    Tile *target_tile = &game->map.tiles[target_y][target_x];

    if (can_move(unit, target_tile, &game->player)) {
        if (target_tile->unit != NULL && target_tile->unit->owner != &game->player) {
            resolve_combat(unit, target_tile->unit);
            unit->pm = 0; 
            sprintf(console_status, "Combat ! PV Vous: %d | PV Ennemi: %d", unit->hp, target_tile->unit->hp);
            
            // Nettoyage de l'ennemi s'il meurt
            if (target_tile->unit->hp <= 0) target_tile->unit = NULL;
            // NOUVEAU : Nettoyage de ton unité si TU meurs en attaquant
            if (unit->hp <= 0) game->map.tiles[unit->y][unit->x].unit = NULL;
            
        } 
        else if (target_tile->city != NULL && target_tile->city->owner != &game->player) {
            resolve_combat_city(unit, target_tile->city);
            unit->pm = 0;
            sprintf(console_status, "Attaque de Ville ! PV Vous: %d | PV Ville: %d", unit->hp, target_tile->city->hp);
            
            // suppression de la ville
            if (target_tile->city->hp <= 0) {
                for (int i = 0; i < game->city_count; i++) {
                    if (game->cities[i] == target_tile->city) {
                        free(game->cities[i]); // Libère la RAM
                        game->cities[i] = game->cities[game->city_count - 1]; // Remplace par la dernière
                        game->cities[game->city_count - 1] = NULL;
                        game->city_count--;
                        break;
                    }
                }
                target_tile->city = NULL; // Nettoyage visuel
                strcat(console_status, " -> Ville detruite !");
            }
            // Nettoyage de ton unité si l'unité meurt contre les remparts
            if (unit->hp <= 0) game->map.tiles[unit->y][unit->x].unit = NULL;
        }
        else {
            int cost = get_terrain_pm_cost(target_tile->type);
            if (target_tile->type == TERRAIN_EAU && game->player.unlocked_techs[TECH_BATEAU]) cost = 2;
            unit->pm -= cost;

            game->map.tiles[unit->y][unit->x].unit = NULL;
            unit->x = target_x;
            unit->y = target_y;
            target_tile->unit = unit;

            if (target_tile->camp != NULL && target_tile->camp->destroyed == 0 && unit->type == UNIT_WARRIOR) {
                target_tile->camp->destroyed = 1;
                game->player.gold += 50; 
                sprintf(console_status, "Camp détruit ! +50 Or.");
            } else {
                sprintf(console_status, "Deplacement reussi. PM restants: %d", unit->pm);
            }
        }
    } else {
        if (unit->pm == 0) strcpy(console_status, "Plus de PM ! Passez votre tour.");
        else strcpy(console_status, "Mouvement bloque (Terrain, PM, ou Allie).");
    }
}


// MENUS SDL (une fonction pour chaque, l'une étant la logique du jeu)

static void render_main_menu(SDL_Renderer *renderer, SDL_Texture *bg_texture, TTF_Font *font_normal, TTF_Font *font_large, TTF_Font *font_title, int screen_w, int screen_h, int highlight, bool game_in_progress) {
    if (bg_texture) {
        SDL_RenderCopy(renderer, bg_texture, NULL, NULL);
    } else {
        SDL_SetRenderDrawColor(renderer, 15, 15, 20, 255); 
        SDL_RenderFillRect(renderer, NULL);
    }

    SDL_SetRenderDrawBlendMode(renderer, SDL_BLENDMODE_BLEND);
    SDL_SetRenderDrawColor(renderer, 0, 0, 0, 150);
    SDL_RenderFillRect(renderer, NULL);

    SDL_Color white = {255, 255, 255, 255};
    SDL_Color gold = {255, 215, 0, 255};
    SDL_Color gray = {100, 100, 100, 255};
    SDL_Color highlight_color = {50, 205, 50, 255}; 

    int center_x = screen_w / 2;
    int start_y = screen_h / 4 - 100;

    draw_text_centered(renderer, font_title, "LES INCIVILISES", center_x, start_y, gold);
    draw_text_centered(renderer, font_normal, "Edition Graphique - Moteur SDL2", center_x, start_y + 120, white);

    const char *choices[] = {
        "CONTINUER LA PARTIE",
        "NOUVELLE PARTIE",
        "CHARGER PARTIE",
        "REGLES & COMMANDES",
        "OPTIONS",
        "HISTORIQUE DES SCORES",
        "QUITTER LE JEU"
    };

    int menu_y = screen_h / 2;
    int spacing = 65; 

    for(int i = 0; i < 7; i++) {
        SDL_Color current_color = white;
        
        if (i == 0 && !game_in_progress) current_color = gray;
        else if (i == highlight) current_color = highlight_color;

        char buffer[128];
        if (i == highlight) sprintf(buffer, ">>  %s  <<", choices[i]);
        else sprintf(buffer, "%s", choices[i]);

        int text_w, text_h;
        TTF_SizeText(font_large, buffer, &text_w, &text_h);
        SDL_Rect btn = { (screen_w - text_w - 40) / 2, menu_y + (i * spacing) - 5, text_w + 40, text_h + 10 };
        
        if (i == highlight) {
            SDL_SetRenderDrawColor(renderer, 50, 205, 50, 150); 
            SDL_RenderFillRect(renderer, &btn);
            SDL_SetRenderDrawColor(renderer, 255, 255, 255, 255);
            SDL_RenderDrawRect(renderer, &btn); 
        }
        draw_text_centered(renderer, font_large, buffer, screen_w / 2, menu_y + (i * spacing), current_color);
    }
}

static void render_options_menu(SDL_Renderer *renderer, TTF_Font *font_normal, TTF_Font *font_large, TTF_Font *font_title, int screen_w, int screen_h, int highlight, int diff_level, bool enable_fow_ui) {
    (void)font_normal; 
    SDL_SetRenderDrawColor(renderer, 20, 20, 30, 255); 
    SDL_RenderFillRect(renderer, NULL);

    SDL_Color white = {255, 255, 255, 255};
    SDL_Color gold = {255, 215, 0, 255};
    SDL_Color highlight_color = {50, 205, 50, 255}; 

    int center_x = screen_w / 2;
    draw_text_centered(renderer, font_title, "OPTIONS DE JEU", center_x, 100, gold);

    const char* diff_str = (diff_level == 0) ? "Facile" : (diff_level == 1) ? "Normal" : "Difficile";
    const char* fow_str = enable_fow_ui ? "Actif" : "Inactif";

    char opt1[64], opt2[64], opt3[64];
    sprintf(opt1, "Difficulte : < %s >", diff_str);
    sprintf(opt2, "Brouillard de Guerre : < %s >", fow_str);
    sprintf(opt3, "Retour au Menu Principal");

    const char *choices[] = { opt1, opt2, opt3 };
    int menu_y = screen_h / 2 - 50;

    for(int i = 0; i < 3; i++) {
        SDL_Color color = (i == highlight) ? highlight_color : white;
        char buffer[128];
        if (i == highlight) sprintf(buffer, ">>  %s  <<", choices[i]);
        else sprintf(buffer, "%s", choices[i]);
        draw_text_centered(renderer, font_large, buffer, center_x, menu_y + (i * 60), color);
    }
}

static void render_rules_menu(SDL_Renderer *renderer, TTF_Font *font_normal, TTF_Font *font_title, int screen_w, int screen_h) {
    SDL_SetRenderDrawColor(renderer, 20, 20, 30, 255); 
    SDL_RenderFillRect(renderer, NULL);

    SDL_Color white = {255, 255, 255, 255};
    SDL_Color gold = {255, 215, 0, 255};

    int center_x = screen_w / 2;
    draw_text_centered(renderer, font_title, "REGLES DE L'EMPIRE", center_x, 50, gold);

    int y = 180;
    int spacing = 45;

    draw_text_centered(renderer, font_normal, "- BUT : Developper votre civilisation et survivre aux Barbares.", center_x, y, white); y += spacing;
    draw_text_centered(renderer, font_normal, "- NOURRITURE : Fait croitre la population. Attention a la famine !", center_x, y, white); y += spacing;
    draw_text_centered(renderer, font_normal, "- PRODUCTION : Sert a construire des batiments et recruter des unites.", center_x, y, white); y += spacing;
    draw_text_centered(renderer, font_normal, "- SCIENCE : Debloque de nouvelles technologies cruciales.", center_x, y, white); y += spacing * 2;
    
    draw_text_centered(renderer, font_normal, "=== COMMANDES PRINCIPALES ===", center_x, y, gold); y += spacing + 10;
    draw_text_centered(renderer, font_normal, "[Clic]    : Selectionner une unite ou une ville sur la carte", center_x, y, white); y += spacing;
    draw_text_centered(renderer, font_normal, "[z/q/s/d] : Deplacer votre unite selectionnee (ou attaquer !)", center_x, y, white); y += spacing;
    draw_text_centered(renderer, font_normal, "[f]       : Fonder une ville (necessite un Colon)", center_x, y, white); y += spacing;
    draw_text_centered(renderer, font_normal, "[v]       : Gerer la ville selectionnee", center_x, y, white); y += spacing;
    draw_text_centered(renderer, font_normal, "[t]       : Ouvrir l'arbre des Technologies", center_x, y, white); y += spacing;
    draw_text_centered(renderer, font_normal, "[Maj+s]   : Sauvegarder manuellement la partie en cours", center_x, y, white);

    draw_text_centered(renderer, font_normal, "Appuyez sur [Echap] ou [Entree] pour revenir", center_x, screen_h - 80, gold);
}

static void render_score_history(SDL_Renderer *renderer, TTF_Font *font_normal, TTF_Font *font_title, int screen_w, int screen_h) {
    SDL_SetRenderDrawColor(renderer, 20, 20, 30, 255); 
    SDL_RenderFillRect(renderer, NULL);

    SDL_Color white = {255, 255, 255, 255};
    SDL_Color gold = {255, 215, 0, 255};

    int center_x = screen_w / 2;
    draw_text_centered(renderer, font_title, "HISTORIQUE DES SCORES", center_x, 50, gold);

    int y = 180;
    FILE *f = fopen("score_history.txt", "r");
    if (f) {
        char line[256];
        draw_text_centered(renderer, font_normal, "Voici les dernieres parties enregistrees :", center_x, y, gold);
        y += 60;
        while (fgets(line, sizeof(line), f) && y < screen_h - 100) {
            line[strcspn(line, "\n")] = 0; 
            draw_text_centered(renderer, font_normal, line, center_x, y, white);
            y += 45;
        }
        fclose(f);
    } else {
        draw_text_centered(renderer, font_normal, "Aucun score enregistre pour le moment.", center_x, y, white);
    }

    draw_text_centered(renderer, font_normal, "Appuyez sur [Echap] ou [Entree] pour revenir", center_x, screen_h - 80, gold);
}

static void render_game_over(SDL_Renderer *renderer, TTF_Font *font_normal, TTF_Font *font_title, GameState *game, int screen_w, int screen_h) {
    SDL_SetRenderDrawColor(renderer, 20, 20, 30, 240); 
    SDL_RenderFillRect(renderer, NULL);

    SDL_Color gold = {255, 215, 0, 255};
    SDL_Color red = {255, 50, 50, 255};
    SDL_Color green = {50, 255, 50, 255};
    SDL_Color white = {255, 255, 255, 255};

    int center_x = screen_w / 2;
    int y = screen_h / 3;

    if (game->game_over == 1) {
        draw_text_centered(renderer, font_title, "VICTOIRE !", center_x, y, green);
    } else {
        draw_text_centered(renderer, font_title, "DEFAITE...", center_x, y, red);
    }

    y += 150;
    char buffer[256];
    sprintf(buffer, "Vous avez survecu %d tours sur cette carte.", game->turn);
    draw_text_centered(renderer, font_normal, buffer, center_x, y, white);

    y += 100;

    // --- AFFICHAGE DE LA RAISON DU GAME OVER ---
    char reason_buffer[128];
    SDL_Color reason_color = {200, 200, 200, 255}; // Gris clair

    strcpy(reason_buffer, game->end_reason);
    draw_text_centered(renderer, font_normal, reason_buffer, center_x, y, reason_color);

    y += 80;
    draw_text_centered(renderer, font_normal, "Appuyez sur [Entree] pour enregistrer le score et revenir au menu.", center_x, y, gold);
}

static void render_tech_menu(SDL_Renderer *renderer, TTF_Font *font, GameState *game, int screen_w, int screen_h) {
    SDL_SetRenderDrawColor(renderer, 0, 0, 0, 240);
    SDL_RenderFillRect(renderer, NULL);

    SDL_Color white = {255, 255, 255, 255};
    SDL_Color cyan = {0, 200, 255, 255};
    SDL_Color green = {50, 205, 50, 255};
    SDL_Color gray = {150, 150, 150, 255};
    SDL_Color title_color = {180, 180, 220, 255};

    int x = screen_w / 2 - 500;
    int y = 50;
    char buffer[256];

    draw_text(renderer, font, "=== ARBRE TECHNOLOGIQUE ===", x, y, title_color);
    y += 60;
    sprintf(buffer, "Cagnotte de Science : %d", game->player.science);
    draw_text(renderer, font, buffer, x, y, white);
    y += 45;

    if (game->player.current_research != TECH_NONE) {
        sprintf(buffer, "Recherche en cours : [%s] (%d / %d Science)", 
                TECH_CATALOG[game->player.current_research].name,
                game->player.science,
                TECH_CATALOG[game->player.current_research].cost);
        draw_text(renderer, font, buffer, x, y, green);
    } else {
        draw_text(renderer, font, "Aucune recherche en cours.", x, y, gray);
    }
    y += 70;
    draw_text(renderer, font, "Technologies disponibles a la recherche :", x, y, title_color);
    y += 50;

    int options_dispo = 0;
    for (int i = 0; i < MAX_TECHNOLOGIES; i++) {
        if (game->player.unlocked_techs[i]) {
            // Technologie déjà acquise
            sprintf(buffer, "[X] %s (Acquis)", TECH_CATALOG[i].name);
            draw_text(renderer, font, buffer, x, y, gray);
            y += 40;
        } else if (can_research(&game->player, i)) {
            // Technologie disponible à la recherche
            const char* desc = "";
            switch(i) {
                case TECH_CHASSE: desc = "+1 Nourriture sur Foret"; break;
                case TECH_AGRICULTURE: desc = "+10% Nourriture globale"; break;
                case TECH_ARTISANAT: desc = "Debloque Guerrier/Archer, +10% Prod"; break;
                case TECH_ECRITURE: desc = "Debloque Bibliotheque, +10% Science"; break;
                case TECH_EQUITATION: desc = "Debloque Cavalier, +1 PM"; break;
                case TECH_IRRIGATION: desc = "+20% Nourriture supplementaire"; break;
                case TECH_MAÇONNERIE: desc = "Debloque Muraille (Double PV ville)"; break;
                case TECH_COMMERCE: desc = "Debloque Marche, +10% Or"; break;
                case TECH_BATEAU: desc = "Traverser l'Eau (Cout: 2 PM)"; break;
            }
            sprintf(buffer, "[%d] %s (Cout: %d) -> %s", i, TECH_CATALOG[i].name, TECH_CATALOG[i].cost, desc);
            draw_text(renderer, font, buffer, x, y, cyan);
            y += 40;
            options_dispo++;
        } else {
            // Technologie bloquée
            char req_str[128] = "";
            int req1 = TECH_CATALOG[i].req1;
            int req2 = TECH_CATALOG[i].req2;

            if (req1 != TECH_NONE && req2 != TECH_NONE) {
                sprintf(req_str, "Prerequis : %s et %s", TECH_CATALOG[req1].name, TECH_CATALOG[req2].name);
            } else if (req1 != TECH_NONE) {
                sprintf(req_str, "Prerequis : %s", TECH_CATALOG[req1].name);
            } else if (req2 != TECH_NONE) {
                sprintf(req_str, "Prerequis : %s", TECH_CATALOG[req2].name);
            }

            sprintf(buffer, "[_] %s [BLOQUE] - %s", TECH_CATALOG[i].name, req_str);
            draw_text(renderer, font, buffer, x, y, gray);
            y += 40;
        }
    }
    
    if (options_dispo == 0) {
        draw_text(renderer, font, "Aucune nouvelle technologie disponible (verifiez prerequis).", x, y, gray);
    }

    draw_text_centered(renderer, font, "[Echap] Retour au jeu", screen_w / 2, screen_h - 80, white);
}

static void render_city_menu(SDL_Renderer *renderer, TTF_Font *font, GameState *game, int sel_city_idx, int screen_w, int screen_h) {
    SDL_SetRenderDrawColor(renderer, 0, 0, 0, 240);
    SDL_RenderFillRect(renderer, NULL);

    SDL_Color white = {255, 255, 255, 255};
    SDL_Color gold = {255, 215, 0, 255};
    SDL_Color action_color = {255, 165, 0, 255};
    SDL_Color title_color = {180, 180, 220, 255};
    SDL_Color red = {255, 50, 50, 255};
    SDL_Color gray = {150, 150, 150, 255};

    int x = screen_w / 2 - 500;
    int y = 40;
    char buffer[256];

    if (sel_city_idx == -1 || !game->cities[sel_city_idx]) {
        draw_text(renderer, font, "Erreur : Aucune ville selectionnee.", x, y, white);
        return;
    } 
    
    City *city = game->cities[sel_city_idx];

    int seuil_croissance = 20 + (10 * city->population);
    if (city->has_granary) seuil_croissance = (seuil_croissance * 2) / 3;
    float wonder_mult = wonder_growth_threshold_multiplier(game);
    if (wonder_mult != 1.0f) seuil_croissance = (int)(seuil_croissance * wonder_mult + 0.5f);
    if (seuil_croissance < 1) seuil_croissance = 1;

    // --- 1. INFOS DE LA VILLE ---
    sprintf(buffer, "=== GESTION DE LA VILLE - Position [%d, %d] ===", city->x, city->y);
    draw_text(renderer, font, buffer, x, y, title_color); y += 45;
    
    sprintf(buffer, "Population : %d    |    PV : %d", city->population, city->hp);
    draw_text(renderer, font, buffer, x, y, white); y += 40;
    
    if (city->food_stored < 0) {
        sprintf(buffer, "[!] ALERTE FAMINE : Deficit de %d Nourriture ! La population baisse.", city->food_stored);
        draw_text(renderer, font, buffer, x, y, red);
    } else {
        sprintf(buffer, "Nourriture : %d / %d avant croissance", city->food_stored, seuil_croissance);
        draw_text(renderer, font, buffer, x, y, white);
    }
    y += 40;
    sprintf(buffer, "Production stockee : %d", city->production_stored);
    draw_text(renderer, font, buffer, x, y, white); y += 55;


    // --- 2. INFRASTRUCTURES ---
    draw_text(renderer, font, "-- INFRASTRUCTURES --", x, y, title_color); y += 40;
    
    char infra_str[512] = "Batiments construits : ";
    int has_buildings = 0;
    if (city->has_granary) { strcat(infra_str, "Grenier "); has_buildings = 1; }
    if (city->has_workshop) { strcat(infra_str, "Atelier "); has_buildings = 1; }
    if (city->has_library) { strcat(infra_str, "Bibliotheque "); has_buildings = 1; }
    if (city->has_market) { strcat(infra_str, "Marche "); has_buildings = 1; }
    if (city->has_barrack) { strcat(infra_str, "Caserne "); has_buildings = 1; }
    if (city->has_wall) { strcat(infra_str, "Muraille "); has_buildings = 1; }
    if (!has_buildings) strcat(infra_str, "Aucun");
    draw_text(renderer, font, infra_str, x, y, white); y += 40;

    int has_wonder = 0;
    char wonder_str[512] = "Merveilles hebergees : ";
    for (int w = 0; w < NUM_WONDERS; w++) {
        if (game->wonder_built[w] && game->wonder_host_city_id[w] == city->id) {
            strcat(wonder_str, wonder_name((WonderType)w));
            strcat(wonder_str, " ");
            has_wonder = 1;
        }
    }
    if (has_wonder) { draw_text(renderer, font, wonder_str, x, y, gold); y += 40; }
    y += 15;


    // --- 3. PROJET ACTUEL ---
    draw_text(renderer, font, "-- PROJET ACTUEL --", x, y, title_color); y += 40;
    
    if (city->current_project == PROJECT_NONE) {
        draw_text(renderer, font, "Aucun projet en cours.", x, y, white);
    } else if (city->current_project >= PROJECT_WONDER_PYRAMIDS && city->current_project <= PROJECT_WONDER_HANGING_GARDEN) {
        WonderType w = project_to_wonder(city->current_project);
        sprintf(buffer, "MERVEILLE : %s (%d / %d Prod)", wonder_name(w), city->production_stored, wonder_cost(w));
        draw_text(renderer, font, buffer, x, y, gold);
    } else {
        sprintf(buffer, "Production : %s (%d / %d Prod)", get_project_name(city->current_project), city->production_stored, get_project_cost(city->current_project));
        draw_text(renderer, font, buffer, x, y, white);
    }
    y += 55;


    // --- 4. CHOIX DE PRODUCTION ---
    draw_text(renderer, font, "-- NOUVEAU PROJET --", x, y, title_color); y += 40;
    draw_text(renderer, font, "Que voulez-vous produire ?", x, y, white); y += 35;
    
    draw_text(renderer, font, "[0] Rien (Garder le projet actuel ou Annuler)", x, y, white); y += 35;
    draw_text(renderer, font, "[1] Colon (50 Prod)       - Fonde une nouvelle ville", x, y, action_color); y += 35;

    if (city->has_barrack && city->owner && city->owner->unlocked_techs[TECH_ARTISANAT]) draw_text(renderer, font, "[2] Guerrier (40 Prod)    - Unite militaire (Force: 3, PM: 3)", x, y, action_color);
    else draw_text(renderer, font, "[2] [BLOQUE] Guerrier     - Necessite Caserne + Tech Artisanat", x, y, gray);
    y += 35;

    if (!city->has_granary)  { draw_text(renderer, font, "[3] Grenier (30 Prod)     - Bonus : +3 Nourriture/tour", x, y, action_color); y += 35; }
    if (!city->has_workshop) { draw_text(renderer, font, "[4] Atelier (40 Prod)     - Bonus : +3 Production/tour", x, y, action_color); y += 35; }
    if (!city->has_library) {
        if (city->owner && city->owner->unlocked_techs[TECH_ECRITURE]) draw_text(renderer, font, "[5] Bibliotheque (50 Prod)- Bonus : +4 Science/tour", x, y, action_color);
        else draw_text(renderer, font, "[5] [BLOQUE] Bibliotheque - Necessite Tech Ecriture", x, y, gray);
        y += 35; 
    }
    
    if (!city->has_market) {
        if (city->owner && city->owner->unlocked_techs[TECH_COMMERCE]) draw_text(renderer, font, "[6] Marche (40 Prod)      - Bonus : +3 Or/tour", x, y, action_color);
        else draw_text(renderer, font, "[6] [BLOQUE] Marche       - Necessite Tech Commerce", x, y, gray);
        y += 35; 
    }

    if (!city->has_barrack)  { draw_text(renderer, font, "[7] Caserne (60 Prod)     - Bonus : Permet de recruter des Guerriers", x, y, action_color); y += 35; }
    
    if (!city->has_wall) {
        if (city->owner && city->owner->unlocked_techs[TECH_MAÇONNERIE]) draw_text(renderer, font, "[8] Muraille (80 Prod)    - Bonus : Double les PV max de la ville", x, y, action_color);
        else draw_text(renderer, font, "[8] [BLOQUE] Muraille     - Necessite Tech Maconnerie", x, y, gray);
        y += 35; 
    }

    if (city->has_barrack && city->owner && city->owner->unlocked_techs[TECH_CHASSE]) {
        draw_text(renderer, font, "[9] Archer (55 Prod)      - Unite avancee (Force: 4, PM: 2)", x, y, action_color);
    } else {
        draw_text(renderer, font, "[9] [BLOQUE] Archer       - Necessite Caserne + Tech Chasse", x, y, gray);
    }
    y += 35;

    if (city->has_barrack && city->owner && city->owner->unlocked_techs[TECH_EQUITATION]) {
        draw_text(renderer, font, "[c] Cavalier (60 Prod)    - Unite avancee (Force: 4, PM: 5)", x, y, action_color);
    } else {
        draw_text(renderer, font, "[c] [BLOQUE] Cavalier     - Necessite Caserne + Tech Equitation", x, y, gray);
    }
    y += 55;

    // --- 5. MERVEILLES DU MONDE ---
    draw_text(renderer, font, "-- MERVEILLES DU MONDE --", x, y, title_color); y += 40;

    static const struct { WonderType w; char key; } MENU[NUM_WONDERS] = {
        { WONDER_PYRAMIDS,       'P' },
        { WONDER_GREAT_LIBRARY,  'B' },
        { WONDER_COLISEUM,       'C' },
        { WONDER_BABEL,          'T' },
        { WONDER_HANGING_GARDEN, 'J' }
    };

    for (int i = 0; i < NUM_WONDERS; i++) {
        WonderType w = MENU[i].w;
        char key = MENU[i].key;
        
        if (wonder_is_built(game, w)) {
            if (game->wonder_host_city_id[w] == city->id) sprintf(buffer, "[%c] [DEJA ICI] %s", key, wonder_name(w));
            else sprintf(buffer, "[%c] [DEJA BATIE AILLEURS] %s", key, wonder_name(w));
            draw_text(renderer, font, buffer, x, y, gray);
        } else {
            int tx, ty;
            if (!wonder_can_build_in_city(game, city, w, &tx, &ty)) {
                const char* tn = (WONDER_CATALOG[w].terrain == TERRAIN_DESERT) ? "Desert" :
                                 (WONDER_CATALOG[w].terrain == TERRAIN_TOUNDRA) ? "Toundra" :
                                 (WONDER_CATALOG[w].terrain == TERRAIN_PLAINE) ? "Plaine" :
                                 (WONDER_CATALOG[w].terrain == TERRAIN_MONTAGNE) ? "Montagne" : "Foret";
                sprintf(buffer, "[%c] [BLOQUE] %s (necessite %s dans le rayon)", key, wonder_name(w), tn);
                draw_text(renderer, font, buffer, x, y, gray);
            } else {
                sprintf(buffer, "[Maj + %c] %s (%d Prod) - %s", key, wonder_name(w), wonder_cost(w), WONDER_CATALOG[w].effect_desc);
                draw_text(renderer, font, buffer, x, y, gold);
            }
        }
        y += 35;
    }

    draw_text_centered(renderer, font, "[Echap] Retour au jeu", screen_w / 2, screen_h - 60, white);
}

// Fonction locale qui va draw le dashboard
static void render_dashboard(SDL_Renderer *renderer, TTF_Font *font, GameState *game, int sel_unit_idx, int sel_city_idx, int screen_w, int screen_h, int nb_turns) {
    int panel_w = 420; 
    int panel_x = screen_w - panel_w; 
    
    SDL_Rect panel = { panel_x, 0, panel_w, screen_h };
    SDL_SetRenderDrawColor(renderer, 30, 30, 40, 230); 
    SDL_RenderFillRect(renderer, &panel);

    SDL_SetRenderDrawColor(renderer, 100, 100, 120, 255);
    SDL_RenderDrawLine(renderer, panel_x, 0, panel_x, screen_h);

    SDL_Color white = {255, 255, 255, 255};
    SDL_Color gold_color = {255, 215, 0, 255};       
    SDL_Color science_color = {0, 200, 255, 255};    
    SDL_Color hp_color = {50, 205, 50, 255};         
    SDL_Color pm_color = {135, 206, 235, 255};         
    SDL_Color action_color = {255, 165, 0, 255};     
    SDL_Color title_color = {180, 180, 220, 255};    

    char buffer[128];
    int start_x = panel_x + 20;
    int current_y = 30;
    int spacing = 40; 

    // --- Calcul des prévisions et affichage ---
    int proj_gold_gain = 0, proj_gold_maint = 0, proj_science = 0;
    compute_turn_predictions(game, &proj_gold_gain, &proj_gold_maint, &proj_science);

    // Stats globales
    draw_text(renderer, font, "=== VOTRE EMPIRE ===", start_x, current_y, title_color);
    current_y += spacing + 10;
    sprintf(buffer, "Tour actuel : %d", game->turn);
    draw_text(renderer, font, buffer, start_x, current_y, white);
    current_y += spacing;

    sprintf(buffer, "Or : %d (+%d gains, -%d maint)", game->player.gold, proj_gold_gain, proj_gold_maint);
    draw_text(renderer, font, buffer, start_x, current_y, gold_color);
    current_y += spacing;
    
    sprintf(buffer, "Science : %d (+%d/tour)", game->player.science, proj_science);
    draw_text(renderer, font, buffer, start_x, current_y, science_color);
    current_y += spacing * 2; 

    // Selection du joueur
    draw_text(renderer, font, "=== SELECTION ===", start_x, current_y, title_color);
    current_y += spacing + 10;

    if (sel_unit_idx != -1 && game->player_units[sel_unit_idx]) {
        Unit *u = game->player_units[sel_unit_idx];
        draw_text(renderer, font, "Cible : Unite alliee", start_x, current_y, white);
        current_y += spacing;
        sprintf(buffer, "Type : %s | Pos : (%d,%d)", unit_label(u->type), u->x, u->y);
        draw_text(renderer, font, buffer, start_x, current_y, white);
        current_y += spacing;
        
        sprintf(buffer, "Sante : %d PV", u->hp);
        draw_text(renderer, font, buffer, start_x, current_y, hp_color);
        current_y += spacing;
        
        sprintf(buffer, "Mouvement : %d PM", u->pm);
        draw_text(renderer, font, buffer, start_x, current_y, pm_color);
        current_y += spacing;

    } else if (sel_city_idx != -1 && game->cities[sel_city_idx]) {
        City *c = game->cities[sel_city_idx];
        draw_text(renderer, font, "Cible : Ville alliee", start_x, current_y, white);
        current_y += spacing;
        sprintf(buffer, "Pos : (%d,%d) | Pop : %d", c->x, c->y, c->population);
        draw_text(renderer, font, buffer, start_x, current_y, white);
        current_y += spacing;
        sprintf(buffer, "Sante : %d PV", c->hp);
        draw_text(renderer, font, buffer, start_x, current_y, hp_color);
        current_y += spacing;
    } else {
        draw_text(renderer, font, "Aucune selection.", start_x, current_y, white);
    }

    // --- ALERTES D'ÉTAT ---
    current_y += spacing;
    
    // Alerte Faillite
    if (game->consecutive_zero_production_turns >= 3) {
        SDL_Color red = {255, 50, 50, 255};
        sprintf(buffer, "[!] FAILLITE DANS %d TOURS", 5 - game->consecutive_zero_production_turns);
        draw_text(renderer, font, buffer, start_x, current_y, red);
        current_y += spacing;
    }

    // Alerte Temps restant
    if (nb_turns - game->turn <= 10) {
        SDL_Color yellow = {255, 255, 100, 255};
        sprintf(buffer, "[!] TEMPS : %d tours restants", nb_turns - game->turn);
        draw_text(renderer, font, buffer, start_x, current_y, yellow);
        current_y += spacing;
    }

    // Alerte Dernière Ville
    if (game->city_count == 1 && game->cities[0] != NULL && game->cities[0]->hp < (game->cities[0]->population * 5)) {
        SDL_Color red = {255, 50, 50, 255};
        draw_text(renderer, font, "[!] DERNIERE VILLE ATTAQUEE !", start_x, current_y, red);
        current_y += spacing;
    }
    
    current_y = screen_h - 260; 
    draw_text(renderer, font, "=== ACTIONS ===", start_x, current_y, title_color);
    current_y += spacing + 10;

    if (sel_unit_idx != -1) {
        draw_text(renderer, font, "[Z,Q,S,D] Bouger / Attaquer", start_x, current_y, action_color);
        current_y += spacing;
        if (game->player_units[sel_unit_idx]->type == UNIT_SETTLER) draw_text(renderer, font, "[F] Fonder une ville", start_x, current_y, action_color);
    } else if (sel_city_idx != -1) {
        draw_text(renderer, font, "[V] Gerer la ville", start_x, current_y, action_color);
    }

    current_y = screen_h - 100;
    SDL_RenderDrawLine(renderer, panel_x + 20, current_y - 15, screen_w - 20, current_y - 15);
    draw_text(renderer, font, "[N] Fin Tour | [T] Techs", start_x, current_y, white);
    current_y += spacing;
    draw_text(renderer, font, "[Maj+S] Save | [Echap] Menu", start_x, current_y, white);
}

void run_sdl_mode(GameState *game, int nb_turns, bool enable_fow_ui, int* city_id_counter) {

    // --- INITIALISATION DE LA SDL ---
    if (SDL_Init(SDL_INIT_VIDEO) != 0) {
        fprintf(stderr, "[SDL2 ERROR] Impossible d'initialiser la vidéo : %s\n", SDL_GetError());
        return;
    }
    // Force la SDL à garder les pixels carrés et nets lors des redimensionnements
    SDL_SetHint(SDL_HINT_RENDER_SCALE_QUALITY, "0");

    if (TTF_Init() == -1) {
        fprintf(stderr, "[SDL_ttf ERROR] Impossible d'initialiser SDL_ttf: %s\n", TTF_GetError());
        SDL_Quit();
        return;
    }

    if (!(IMG_Init(IMG_INIT_PNG | IMG_INIT_JPG) & (IMG_INIT_PNG | IMG_INIT_JPG))) {
        fprintf(stderr, "[SDL_image ERROR] : %s\n", IMG_GetError());
    }

    char *base_path = SDL_GetBasePath();
    char font_path[1024];
    snprintf(font_path, sizeof(font_path), "%s../assets/font.ttf", base_path);
    char bg_path[1024];
    snprintf(bg_path, sizeof(bg_path), "%s../assets/menu_bg.png", base_path);

    // CHARGEMENT DES ASSETS
    char main_path[1024], winter_path[1024], build_path[1024], horse_path[1024];
    snprintf(main_path, sizeof(main_path), "%s../assets/AllAssetsPreview.png", base_path);
    snprintf(winter_path, sizeof(winter_path), "%s../assets/WinterDeadTrees.png", base_path);
    snprintf(build_path, sizeof(build_path), "%s../assets/AllBuildings-Preview.png", base_path);
    snprintf(horse_path, sizeof(horse_path), "%s../assets/horse.png", base_path);

    SDL_free(base_path);

    // fonts pour le texte du jeu
    TTF_Font *font_normal = TTF_OpenFont(font_path, 28);
    TTF_Font *font_large = TTF_OpenFont(font_path, 40);
    TTF_Font *font_title = TTF_OpenFont(font_path, 90); 

    if (!font_normal || !font_large || !font_title) { 
        fprintf(stderr, "[SDL_ttf ERROR] Erreur chargement font : %s\n", TTF_GetError());
        TTF_Quit(); SDL_Quit(); return; 
    }

    SDL_DisplayMode dm;
    SDL_GetCurrentDisplayMode(0, &dm);
    
    int screen_w = dm.w;
    int screen_h = dm.h;

    // Créer la fenêtre en plein écran fenêtré (Borderless)
    SDL_Window *window = SDL_CreateWindow("Civilization - Edition Graphique",
                                          SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED,
                                          screen_w, screen_h, SDL_WINDOW_SHOWN | 
                                          SDL_WINDOW_MAXIMIZED | SDL_WINDOW_FULLSCREEN_DESKTOP);

    SDL_Renderer *renderer = SDL_CreateRenderer(window, -1, SDL_RENDERER_ACCELERATED | SDL_RENDERER_PRESENTVSYNC);
    
    // Chargement du fond
    SDL_Texture *bg_texture = IMG_LoadTexture(renderer, bg_path);
    if (!bg_texture) {
        fprintf(stderr, "[SDL_image ERROR] Impossible de charger le fond : %s\n", IMG_GetError());
    }

    // --- CHARGEMENT DES ASSETS DU JEU ---
    SDL_Texture *tex_main = IMG_LoadTexture(renderer, main_path);
    SDL_Texture *tex_winter = IMG_LoadTexture(renderer, winter_path);
    SDL_Texture *tex_buildings = IMG_LoadTexture(renderer, build_path);
    SDL_Texture *tex_horse = IMG_LoadTexture(renderer, horse_path);

    if (!tex_main) printf("ERREUR AllAssetsPreview : %s\n", IMG_GetError());
    if (!tex_winter) printf("ERREUR WinterDeadTrees : %s\n", IMG_GetError());
    if (!tex_buildings) printf("ERREUR AllBuildings-Preview : %s\n", IMG_GetError());
    if (!tex_horse) printf("ERREUR horse : %s\n", IMG_GetError());

    int camera_x = 0;
    int camera_y = 0;
    int camera_speed = 32; 

    SDL_SetRenderDrawBlendMode(renderer, SDL_BLENDMODE_BLEND);

    printf("\n=======================================================\n");
    printf("[SDL2 MOTEUR] Fenêtre graphique initialisée avec succès !\n");
    printf("-> Utilisez votre Souris dans la fenêtre pour sélectionner.\n");
    printf("-> Regardez votre Terminal pour lire les informations d'état.\n");
    printf("=======================================================\n");

    int sel_unit_idx = -1;
    int sel_city_idx = -1;
    char console_status[512] = "Mode Graphique actif. Cliquez sur une entite.";

    bool game_in_progress = false;
    FILE *tmp_file = fopen("current_game.tmp", "r");
    if (tmp_file) {
        game_in_progress = true;
        fclose(tmp_file);
    }

    SDL_ActiveView current_view = VIEW_MAIN_MENU;
    int main_menu_highlight = game_in_progress ? 0 : 1; 
    int options_highlight = 0;
    int diff_level = 1; 

    SDL_Event event;
    
    while (game->game_over != 3) {

        // Vérification des conditions de victoire/défaite
        if ((game->game_over == 1 || game->game_over == 2) && current_view == VIEW_GAME) {
            current_view = VIEW_GAME_OVER;
        }

        if (sel_city_idx >= game->city_count || (sel_city_idx != -1 && game->cities[sel_city_idx] == NULL)) {
            sel_city_idx = -1;
        }

        // A. TRAITEMENT DES INPUTS ET DES ÉVÉNEMENTS
        while (SDL_PollEvent(&event)) {
            if (event.type == SDL_QUIT) game->game_over = 3; 
            
            // Écoute du clavier
            else if (event.type == SDL_KEYDOWN) {
                if (current_view == VIEW_MAIN_MENU) {
                    if (event.key.keysym.sym == SDLK_UP || event.key.keysym.sym == SDLK_z) {
                        main_menu_highlight = (main_menu_highlight <= 0) ? 6 : main_menu_highlight - 1;
                        if (!game_in_progress && main_menu_highlight == 0) main_menu_highlight = 6; 
                    }
                    else if (event.key.keysym.sym == SDLK_DOWN || event.key.keysym.sym == SDLK_s) {
                        main_menu_highlight = (main_menu_highlight >= 6) ? 0 : main_menu_highlight + 1;
                        if (!game_in_progress && main_menu_highlight == 0) main_menu_highlight = 1; 
                    }
                    else if (event.key.keysym.sym == SDLK_RETURN) {
                        if (main_menu_highlight == 0 && game_in_progress) {
                            if (load_game("current_game.tmp", game)) current_view = VIEW_GAME;
                        }
                        else if (main_menu_highlight == 1) { cleanup_game(game); memset(game, 0, sizeof(GameState)); init_game(game, 50, 30, (int)time(NULL), 3, 0); current_view = VIEW_GAME; }
                        else if (main_menu_highlight == 2) { if (load_game("savegame.sav", game)) current_view = VIEW_GAME; }
                        else if (main_menu_highlight == 3) current_view = VIEW_RULES_MENU;
                        else if (main_menu_highlight == 4) current_view = VIEW_OPTIONS_MENU;
                        else if (main_menu_highlight == 5) current_view = VIEW_SCORE_HISTORY; 
                        else if (main_menu_highlight == 6) game->game_over = 3;
                    }
                } 
                
                else if (current_view == VIEW_OPTIONS_MENU) {
                    if (event.key.keysym.sym == SDLK_UP || event.key.keysym.sym == SDLK_z) options_highlight = (options_highlight <= 0) ? 2 : options_highlight - 1;
                    else if (event.key.keysym.sym == SDLK_DOWN || event.key.keysym.sym == SDLK_s) options_highlight = (options_highlight >= 2) ? 0 : options_highlight + 1;
                    else if (event.key.keysym.sym == SDLK_RETURN) {
                        if (options_highlight == 0) diff_level = (diff_level + 1) % 3;
                        else if (options_highlight == 1) enable_fow_ui = !enable_fow_ui;
                        else if (options_highlight == 2) current_view = VIEW_MAIN_MENU;
                    } else if (event.key.keysym.sym == SDLK_ESCAPE) current_view = VIEW_MAIN_MENU;
                }

                else if (current_view == VIEW_RULES_MENU || current_view == VIEW_SCORE_HISTORY) {
                    if (event.key.keysym.sym == SDLK_ESCAPE || event.key.keysym.sym == SDLK_RETURN) current_view = VIEW_MAIN_MENU;
                }

                else if (current_view == VIEW_GAME_OVER) {
                    if (event.key.keysym.sym == SDLK_RETURN) {
                        int score = calculate_final_score(game);
                        save_final_score("Joueur_SDL", score, game->turn, game->game_over);
                        
                        game->game_over = 0; 
                        game_in_progress = false;
                        cleanup_game(game);
                        memset(game, 0, sizeof(GameState));
                        current_view = VIEW_MAIN_MENU;
                    }
                }

                else if (current_view == VIEW_TECH_MENU) {
                    if (event.key.keysym.sym == SDLK_ESCAPE || event.key.keysym.sym == SDLK_x) current_view = VIEW_GAME;
                    else if (event.key.keysym.sym >= SDLK_0 && event.key.keysym.sym <= SDLK_8) {
                        int t_idx = event.key.keysym.sym - SDLK_0;
                        if (can_research(&game->player, t_idx)) { 
                            start_research(&game->player, t_idx); 
                            sprintf(console_status, "Nouvelle recherche lancee !");
                            current_view = VIEW_GAME; 
                        }
                    }
                }

                else if (current_view == VIEW_CITY_MENU) {
                    if (event.key.keysym.sym == SDLK_ESCAPE) {
                        current_view = VIEW_GAME;
                    } else {
                        City *city = game->cities[sel_city_idx];
                        bool shift = SDL_GetModState() & KMOD_SHIFT;
                        int sym = event.key.keysym.sym;
                        
                        WonderType picked_wonder = WONDER_NONE;
                        if (shift && sym == SDLK_p) picked_wonder = WONDER_PYRAMIDS;
                        else if (shift && sym == SDLK_b) picked_wonder = WONDER_GREAT_LIBRARY;
                        else if (shift && sym == SDLK_c) picked_wonder = WONDER_COLISEUM;
                        else if (shift && sym == SDLK_t) picked_wonder = WONDER_BABEL;
                        else if (shift && sym == SDLK_j) picked_wonder = WONDER_HANGING_GARDEN;

                        if (picked_wonder != WONDER_NONE) {
                            if (wonder_is_built(game, picked_wonder)) {
                                sprintf(console_status, "Impossible : Merveille deja construite.");
                            } else {
                                int tx, ty;
                                if (!wonder_can_build_in_city(game, city, picked_wonder, &tx, &ty)) {
                                    sprintf(console_status, "Impossible : terrain incompatible dans le rayon.");
                                } else {
                                    city->current_project = wonder_to_project(picked_wonder);
                                    city->production_stored = 0;
                                    sprintf(console_status, "Projet defini : %s", wonder_name(picked_wonder));
                                    current_view = VIEW_GAME;
                                }
                            }
                        } else {
                            if (sym == SDLK_0) {
                                sprintf(console_status, "Action annulee.");
                                current_view = VIEW_GAME;
                            } else if (sym == SDLK_1) {
                                city->current_project = PROJECT_SETTLER; city->production_stored = 0; sprintf(console_status, "Projet defini : Colon"); current_view = VIEW_GAME;
                            } else if (sym == SDLK_2) {
                                if (city->has_barrack && game->player.unlocked_techs[TECH_ARTISANAT]) { city->current_project = PROJECT_WARRIOR; city->production_stored = 0; sprintf(console_status, "Projet defini : Guerrier"); current_view = VIEW_GAME; }
                                else { sprintf(console_status, "Impossible : Caserne + Artisanat requis !"); }
                            } else if (sym == SDLK_3) { city->current_project = PROJECT_GRANARY; city->production_stored = 0; sprintf(console_status, "Projet defini : Grenier"); current_view = VIEW_GAME; }
                            else if (sym == SDLK_4) { city->current_project = PROJECT_WORKSHOP; city->production_stored = 0; sprintf(console_status, "Projet defini : Atelier"); current_view = VIEW_GAME; }
                            else if (sym == SDLK_5) { 
                                if (city->owner->unlocked_techs[TECH_ECRITURE]) { city->current_project = PROJECT_LIBRARY; city->production_stored = 0; sprintf(console_status, "Projet defini : Bibliotheque"); current_view = VIEW_GAME; }
                                else { sprintf(console_status, "Impossible : Ecriture requise !"); }
                            }
                            else if (sym == SDLK_6) { 
                                if (city->owner->unlocked_techs[TECH_COMMERCE]) { city->current_project = PROJECT_MARKET; city->production_stored = 0; sprintf(console_status, "Projet defini : Marche"); current_view = VIEW_GAME; }
                                else { sprintf(console_status, "Impossible : Commerce requis !"); }
                            }
                            else if (sym == SDLK_7) { city->current_project = PROJECT_BARRACKS; city->production_stored = 0; sprintf(console_status, "Projet defini : Caserne"); current_view = VIEW_GAME; }
                            else if (sym == SDLK_8) { 
                                if (city->owner->unlocked_techs[TECH_MAÇONNERIE]) { city->current_project = PROJECT_WALL; city->production_stored = 0; sprintf(console_status, "Projet defini : Muraille"); current_view = VIEW_GAME; }
                                else { sprintf(console_status, "Impossible : Maconnerie requise !"); }
                            }
                            else if (sym == SDLK_9) {
                                if (city->has_barrack && game->player.unlocked_techs[TECH_CHASSE]) { city->current_project = PROJECT_ARCHER; city->production_stored = 0; sprintf(console_status, "Projet defini : Archer"); current_view = VIEW_GAME; }
                                else { sprintf(console_status, "Impossible : Archer requis Caserne + Chasse !"); }
                            } else if (sym == SDLK_c && !shift) {
                                if (city->has_barrack && game->player.unlocked_techs[TECH_EQUITATION]) { city->current_project = PROJECT_CAVALIER; city->production_stored = 0; sprintf(console_status, "Projet defini : Cavalier"); current_view = VIEW_GAME; }
                                else { sprintf(console_status, "Impossible : Cavalier requis Caserne + Equitation !"); }
                            }
                        }
                    }
                }

                else if (current_view == VIEW_GAME) {
                    switch (event.key.keysym.sym) {
                        case SDLK_ESCAPE: 
                            save_game("current_game.tmp", game); 
                            game_in_progress = true;
                            current_view = VIEW_MAIN_MENU; 
                            break;
                        case SDLK_t: current_view = VIEW_TECH_MENU; break;
                        case SDLK_v: if (sel_city_idx != -1) current_view = VIEW_CITY_MENU; break;
                        case SDLK_f: {
                            if (sel_unit_idx != -1 && game->player_units[sel_unit_idx] && game->player_units[sel_unit_idx]->type == UNIT_SETTLER && game->player_units[sel_unit_idx]->pm > 0) {
                                Unit *settler = game->player_units[sel_unit_idx];
                                
                                if (found_city(settler, &game->map, city_id_counter, game->cities, &game->city_count)) {
                                    
                                    // --- SUCCÈS ---
                                    strcpy(console_status, "Ville fondee avec succes !");

                                    game->player_units[sel_unit_idx] = game->player_units[game->player_unit_count - 1]; 
                                    game->player_units[game->player_unit_count - 1] = NULL; 
                                    game->player_unit_count--; 
                                    
                                    sel_unit_idx = -1; 
                                    
                                } else {
                                    // --- ÉCHEC ---
                                    strcpy(console_status, "Impossible : Eau, Limite atteinte ou Trop proche !");
                                }
                            } else {
                                strcpy(console_status, "Action impossible : Selectionnez un Colon.");
                            }
                            break;
                        }  
                        case SDLK_n: 
                            next_turn(game); 
                            sel_unit_idx = -1; 
                            sel_city_idx = -1;
                            check_victory_and_defeat(game); 
                            if (game->combat_log_ready) {
                                sprintf(console_status, "Tour %d | ENNEMI : %s", game->turn - 1, game->combat_log);
                                game->combat_log_ready = 0; // On reset après affichage
                            } else {
                                sprintf(console_status, "Tour %d termine !", game->turn - 1); 
                            }
                            break;
                        case SDLK_UP:    camera_y -= camera_speed; break;
                        case SDLK_DOWN:  camera_y += camera_speed; break;
                        case SDLK_LEFT:  camera_x -= camera_speed; break;
                        case SDLK_RIGHT: camera_x += camera_speed; break;
                        
                        // Centrer la caméra avec la barre Espace pour éviter de se perdre avec une grande map
                        case SDLK_SPACE:
                            if (sel_unit_idx != -1 && game->player_units[sel_unit_idx]) {
                                camera_x = (game->player_units[sel_unit_idx]->x * TILE_SIZE) - ((screen_w - 420) / 2);
                                camera_y = (game->player_units[sel_unit_idx]->y * TILE_SIZE) - (screen_h / 2);
                                strcpy(console_status, "Camera centree sur l'unite.");
                            } else if (sel_city_idx != -1 && game->cities[sel_city_idx]) {
                                camera_x = (game->cities[sel_city_idx]->x * TILE_SIZE) - ((screen_w - 420) / 2);
                                camera_y = (game->cities[sel_city_idx]->y * TILE_SIZE) - (screen_h / 2);
                                strcpy(console_status, "Camera centree sur la ville.");
                            } else if (game->player_unit_count > 0) {
                                camera_x = (game->player_units[0]->x * TILE_SIZE) - ((screen_w - 420) / 2);
                                camera_y = (game->player_units[0]->y * TILE_SIZE) - (screen_h / 2);
                                strcpy(console_status, "Camera centree sur votre point de depart.");
                            }
                            break;

                        case SDLK_z: 
                            attempt_move_sdl(game, sel_unit_idx, 0, -1, console_status); 
                            if (sel_unit_idx != -1 && game->player_units[sel_unit_idx]->hp <= 0) sel_unit_idx = -1;
                            break;
                        case SDLK_q: 
                            attempt_move_sdl(game, sel_unit_idx, -1, 0, console_status); 
                            if (sel_unit_idx != -1 && game->player_units[sel_unit_idx]->hp <= 0) sel_unit_idx = -1;
                            break;
                        case SDLK_d: 
                            attempt_move_sdl(game, sel_unit_idx, 1, 0, console_status); 
                            if (sel_unit_idx != -1 && game->player_units[sel_unit_idx]->hp <= 0) sel_unit_idx = -1;
                            break;
                        case SDLK_s: 
                            // Sauvegarde manuelle in-game
                            if (SDL_GetModState() & KMOD_SHIFT) {
                                save_game("savegame.sav", game);
                                strcpy(console_status, "Partie sauvegardee avec succes !");
                            } else {
                                attempt_move_sdl(game, sel_unit_idx, 0, 1, console_status); 
                                if (sel_unit_idx != -1 && game->player_units[sel_unit_idx]->hp <= 0) sel_unit_idx = -1;
                            }
                            break;
                        case SDLK_F11: {
                            Uint32 flag = SDL_WINDOW_FULLSCREEN_DESKTOP;
                            bool isFS = SDL_GetWindowFlags(window) & flag;
                            SDL_SetWindowFullscreen(window, isFS ? 0 : flag);
                            SDL_GetWindowSize(window, &screen_w, &screen_h);
                            break;
                        }
                        default: break;
                    }
                    clamp_camera(&camera_x, &camera_y, game->map.width, game->map.height, screen_w, screen_h);
                }
            }
            else if (event.type == SDL_MOUSEMOTION) {
                int mx = event.motion.x;
                int my = event.motion.y;
                if (current_view == VIEW_MAIN_MENU) {
                    int menu_y = screen_h / 2;
                    for(int i = 0; i < 7; i++) {
                        int item_y = menu_y + (i * 65);
                        if (my >= item_y - 10 && my <= item_y + 60 && mx > screen_w/2 - 350 && mx < screen_w/2 + 350) {
                            if (i == 0 && !game_in_progress) continue; 
                            main_menu_highlight = i;
                        }
                    }
                } else if (current_view == VIEW_OPTIONS_MENU) {
                    int menu_y = screen_h / 2 - 50;
                    for(int i = 0; i < 3; i++) {
                        int item_y = menu_y + (i * 60);
                        if (my >= item_y - 10 && my <= item_y + 60 && mx > screen_w/2 - 350 && mx < screen_w/2 + 350) {
                            options_highlight = i;
                        }
                    }
                }
            }
            else if (event.type == SDL_MOUSEBUTTONDOWN) {
                if (event.button.button == SDL_BUTTON_LEFT) {
                    if (current_view == VIEW_MAIN_MENU || current_view == VIEW_OPTIONS_MENU) {
                        SDL_Event enter_event;
                        enter_event.type = SDL_KEYDOWN;
                        enter_event.key.keysym.sym = SDLK_RETURN;
                        SDL_PushEvent(&enter_event);
                    } 
                    else if (current_view == VIEW_GAME) {
                        if (event.button.x >= screen_w - 420) {
                            continue;
                        }

                        int click_grid_x = (event.button.x + camera_x) / TILE_SIZE;
                        int click_grid_y = (event.button.y + camera_y) / TILE_SIZE;
                        if (click_grid_x >= 0 && click_grid_x < game->map.width &&
                            click_grid_y >= 0 && click_grid_y < game->map.height) {
                            bool target_found = false;
                            for (int i = 0; i < game->player_unit_count; i++) {
                                if (game->player_units[i] && game->player_units[i]->hp > 0 &&
                                    game->player_units[i]->x == click_grid_x && game->player_units[i]->y == click_grid_y) {
                                    sel_unit_idx = i; sel_city_idx = -1; target_found = true; break;
                                }
                            }
                            if (!target_found) {
                                for (int i = 0; i < game->city_count; i++) {
                                    if (game->cities[i] && game->cities[i]->hp > 0 &&
                                        game->cities[i]->x == click_grid_x && game->cities[i]->y == click_grid_y) {
                                        sel_city_idx = i; sel_unit_idx = -1; target_found = true; break;
                                    }
                                }
                            }
                            if (!target_found) { sel_unit_idx = -1; sel_city_idx = -1; }
                        }
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
                int s_pos_x = (x * TILE_SIZE) - camera_x;
                int s_pos_y = (y * TILE_SIZE) - camera_y;
                
                // Optimisation : on ne dessine que ce qui est à l'écran
                if (s_pos_x + TILE_SIZE < 0 || s_pos_x > screen_w || s_pos_y + TILE_SIZE < 0 || s_pos_y > screen_h) continue;

                // 1. DESSIN DU TERRAIN (Fond)
                switch (tile->type) {
                    case TERRAIN_EAU:      draw_texture_part(renderer, tex_main, 144, 240, s_pos_x, s_pos_y); break;
                    case TERRAIN_PLAINE:   draw_texture_part(renderer, tex_main, 176, 240, s_pos_x, s_pos_y); break;
                    case TERRAIN_DESERT:   draw_texture_part(renderer, tex_main, 160, 256, s_pos_x, s_pos_y); break;
                    case TERRAIN_FORET:
                        draw_texture_part(renderer, tex_main, 176, 240, s_pos_x, s_pos_y); // Fond plaine
                        draw_texture_part(renderer, tex_main, 16, 240, s_pos_x, s_pos_y);  // Arbre
                        break;
                    case TERRAIN_MONTAGNE:
                        draw_texture_part(renderer, tex_main, 176, 240, s_pos_x, s_pos_y); // Fond plaine
                        draw_texture_part(renderer, tex_main, 80, 176, s_pos_x, s_pos_y);  // Rocher
                        break;
                    case TERRAIN_TOUNDRA:
                        SDL_SetRenderDrawColor(renderer, 240, 240, 250, 255); // Fond Neige générique
                        SDL_Rect bg = { s_pos_x, s_pos_y, TILE_SIZE, TILE_SIZE };
                        SDL_RenderFillRect(renderer, &bg);
                        draw_texture_part(renderer, tex_winter, 32, 0, s_pos_x, s_pos_y);  // Sapin
                        break;
                }

                // 2. DESSIN DES ENTITÉS (Si la zone n'est pas dans le noir total)
                if (tile->visibility != FOG_HIDDEN) {
                    
                    // Ressources spéciales
                    if (tile->special_ressources != SPRES_NONE && sres_visible(&game->player, tile->special_ressources)) {
                        if (tile->special_ressources == SPRES_WHEAT) draw_texture_part(renderer, tex_main, 48, 256, s_pos_x, s_pos_y);
                        else if (tile->special_ressources == SPRES_IRON) draw_texture_part(renderer, tex_main, 128, 176, s_pos_x, s_pos_y);
                        else if (tile->special_ressources == SPRES_SPICES) draw_texture_part(renderer, tex_main, 192, 256, s_pos_x, s_pos_y);
                        else if (tile->special_ressources == SPRES_HORSES) {
                            SDL_Rect src_horse = { 256, 256, 1024, 1024 };
                            
                            SDL_Rect dest_horse = { s_pos_x, s_pos_y, TILE_SIZE+ 20, TILE_SIZE +20};
                            
                            SDL_RenderCopy(renderer, tex_horse, &src_horse, &dest_horse);
                        } else if (tile->special_ressources == SPRES_SPECIMENS) draw_texture_part(renderer, tex_main, 720, 224, s_pos_x, s_pos_y);
                    }

                    // Merveilles
                    if (tile->wonder != WONDER_NONE) {
                        draw_texture_part(renderer, tex_main, 80, 80, s_pos_x, s_pos_y);
                    }

                    // Camp Barbare
                    if (tile->camp && tile->camp->destroyed == 0) {
                        draw_texture_part(renderer, tex_buildings, 112, 112, s_pos_x, s_pos_y);
                    }

                    // Villes du joueur
                    if (tile->city && tile->city->hp > 0) {
                        if (tile->city->has_wall) draw_texture_part(renderer, tex_main, 0, 192, s_pos_x, s_pos_y);
                        else draw_texture_part(renderer, tex_main, 144, 96, s_pos_x, s_pos_y);
                    }

                    // Unités (Joueur et Ennemis)
                    if (tile->unit && tile->unit->hp > 0) {
                        
                        // teinte rouge si c'est une unité ennemie
                        if (tile->unit->owner != &game->player && tile->unit->type != UNIT_BARBARIAN) {
                            SDL_SetTextureColorMod(tex_main, 255, 100, 100); 
                        }

                        if (tile->unit->type == UNIT_SETTLER) draw_texture_part(renderer, tex_main, 272, 0, s_pos_x, s_pos_y);
                        else if (tile->unit->type == UNIT_ARCHER) draw_texture_part(renderer, tex_main, 400, 0, s_pos_x, s_pos_y);
                        else if (tile->unit->type == UNIT_CAVALIER) draw_texture_part(renderer, tex_main, 656, 0, s_pos_x, s_pos_y);
                        else if (tile->unit->type == UNIT_WARRIOR) draw_texture_part(renderer, tex_main, 240, 0, s_pos_x, s_pos_y);
                        else draw_texture_part(renderer, tex_main, 1040, 0, s_pos_x, s_pos_y); // Barbare;

                        // On remet la couleur normale
                        if (tile->unit->owner != &game->player) {
                            SDL_SetTextureColorMod(tex_main, 255, 255, 255);
                        }
                    }
                }

                // 3. BROUILLARD DE GUERRE (overlay par-dessus tout le reste)
                if (tile->visibility == FOG_HIDDEN) { 
                    SDL_SetRenderDrawColor(renderer, 0, 0, 0, 255); 
                    SDL_Rect fog_rect = { s_pos_x, s_pos_y, TILE_SIZE, TILE_SIZE };
                    SDL_RenderFillRect(renderer, &fog_rect); 
                }
                else if (tile->visibility == FOG_EXPLORED) { 
                    SDL_SetRenderDrawColor(renderer, 0, 0, 0, 160); // Filtre gris/noir semi-transparent
                    SDL_Rect fog_rect = { s_pos_x, s_pos_y, TILE_SIZE, TILE_SIZE };
                    SDL_RenderFillRect(renderer, &fog_rect); 
                }

                // Cadre de sélection
                if ((sel_unit_idx != -1 && game->player_units[sel_unit_idx]->x == x && game->player_units[sel_unit_idx]->y == y) ||
                    (sel_city_idx != -1 && game->cities[sel_city_idx]->x == x && game->cities[sel_city_idx]->y == y)) {
                    SDL_SetRenderDrawColor(renderer, 255, 255, 0, 255); 
                    SDL_Rect sel_rect = { s_pos_x, s_pos_y, TILE_SIZE, TILE_SIZE };
                    SDL_RenderDrawRect(renderer, &sel_rect);
                }
            }
        }

        if (current_view == VIEW_GAME) render_dashboard(renderer, font_normal, game, sel_unit_idx, sel_city_idx, screen_w, screen_h, nb_turns);
        else if (current_view == VIEW_MAIN_MENU) render_main_menu(renderer, bg_texture, font_normal, font_large, font_title, screen_w, screen_h, main_menu_highlight, game_in_progress);
        else if (current_view == VIEW_OPTIONS_MENU) render_options_menu(renderer, font_normal, font_large, font_title, screen_w, screen_h, options_highlight, diff_level, enable_fow_ui);
        else if (current_view == VIEW_RULES_MENU) render_rules_menu(renderer, font_normal, font_title, screen_w, screen_h);
        else if (current_view == VIEW_SCORE_HISTORY) render_score_history(renderer, font_normal, font_title, screen_w, screen_h);
        else if (current_view == VIEW_GAME_OVER) render_game_over(renderer, font_normal, font_title, game, screen_w, screen_h);
        else if (current_view == VIEW_TECH_MENU) render_tech_menu(renderer, font_normal, game, screen_w, screen_h);
        else if (current_view == VIEW_CITY_MENU) render_city_menu(renderer, font_normal, game, sel_city_idx, screen_w, screen_h);
        if (current_view == VIEW_GAME) {
            // fond noir semi-transparent en bas de l'écran (qui s'arrête avant le dashboard)
            SDL_Rect status_bg = { 0, screen_h - 40, screen_w - 420, 40 };
            SDL_SetRenderDrawColor(renderer, 0, 0, 0, 200);
            SDL_RenderFillRect(renderer, &status_bg);

            // statut des fights en jaune au centre de la barre
            SDL_Color status_color = {255, 255, 100, 255}; 
            draw_text_centered(renderer, font_normal, console_status, (screen_w - 420) / 2, screen_h - 35, status_color);
        }

        SDL_RenderPresent(renderer);
    }

    // --- SÉCURITÉ ET LIQUIDATION ---
    if(tex_main) SDL_DestroyTexture(tex_main);
    if(tex_winter) SDL_DestroyTexture(tex_winter);
    if(tex_buildings) SDL_DestroyTexture(tex_buildings);
    if(tex_horse) SDL_DestroyTexture(tex_horse);
    SDL_DestroyRenderer(renderer); SDL_DestroyWindow(window);
    if(bg_texture) SDL_DestroyTexture(bg_texture);
    TTF_CloseFont(font_normal); TTF_CloseFont(font_large); TTF_CloseFont(font_title);
    TTF_Quit(); SDL_Quit();
    printf("[SDL MOTEUR] Session graphique libérée proprement.\n");
}