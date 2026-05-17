#include <stdio.h>
#include <stdlib.h>
#include "core/save.h"
#include "core/game.h"
#include "core/barbares.h"
#include <string.h>
#include <time.h>
#include <ncurses.h>
int save_game(const char*filename, GameState * state) {
    if (state == NULL) return 0;


    FILE *file = fopen(filename, "wb");
    if (file == NULL) {
        printf("Erreur : Impossible de créer le fichier de sauvegarde. \n");
        return 0;
    }

    // 1. Sauvegarde des variables de contexte (Le tour, la seed, la fin de partie)
    fwrite(&state->turn, sizeof(int), 1,  file);
    fwrite(&state->seed, sizeof(int), 1, file);
    fwrite(&state->game_over, sizeof(int), 1, file);

    // 2. Sauvegarde du joueur
    fwrite(&state->player, sizeof(Player), 1, file);

    // 3. Sauvegarde des villes
    fwrite(&state->city_count, sizeof(int), 1, file);  // Sauvegarde du nombre de villes
    for (int i=0; i<state->city_count; i++) {
        if(state->cities[i] != NULL) {
            City temp_city = *(state->cities[i]); 
            temp_city.owner = NULL; // On ne sauvegarde pas le pointeur vers le joueur pour éviter les problèmes de référence circulaire
            fwrite(&temp_city, sizeof(City), 1, file);
        }
    }

    //4. Sauvegarde de la structure de la carte
    fwrite(&state->map.width, sizeof(int), 1, file);
    fwrite(&state->map.height, sizeof(int), 1, file);
    fwrite(&state->map.start_x, sizeof(int), 1, file);
    fwrite(&state->map.start_y, sizeof(int), 1, file);

    for (int y=0; y< state->map.height; y++) {
        for (int x=0; x< state->map.width; x++) {
            Tile temp_tile = state->map.tiles[y][x];
            temp_tile.camp = NULL;
            fwrite(&temp_tile, sizeof(Tile), 1, file);
            fwrite(&state->map.tiles[y][x].visibility, sizeof(FogState), 1, file); // Sauvegarde de l'état de visibilité de chaque tuile
        }
    }

    // 4bis. Sauvegarde des campements barbares
    fwrite(&state->barbarian_camp_count, sizeof(int), 1, file);
    fwrite(&state->barbarian_camps_destroyed, sizeof(int), 1, file);
    for (int i = 0; i < state->barbarian_camp_count; ++i) {
        if (state->barbarian_camps) {
            fwrite(&state->barbarian_camps[i], sizeof(BarbarianCamp), 1, file);
        }
    }

    // 5. Sauvegarde des unités du joueur
    fwrite(&state->player_unit_count, sizeof(int), 1, file); // Sauvegarde du nombre d'unités du joueur
    for (int i=0; i<state->player_unit_count; i++) {
        if(state->player_units[i] != NULL) {
            Unit temp_unit = *(state->player_units[i]);
            temp_unit.owner = NULL; // On ne sauvegarde pas le pointeur vers le joueur pour éviter les problèmes de référence circulaire
            fwrite(&temp_unit, sizeof(Unit), 1, file);
        }
    }

    // 6. Sauvegarde des barbares
    fwrite(&state->barbarian_count, sizeof(int), 1, file); // Sauvegarde du nombre de barbares
    for (int i=0; i<state->barbarian_count; i++) {
        if(state->barbarians[i] != NULL) {
            Unit temp_barb = *(state->barbarians[i]);
            temp_barb.owner = NULL; // On ne sauvegarde pas le pointeur vers le joueur pour éviter les problèmes de référence circulaire
            fwrite(&temp_barb, sizeof(Unit), 1, file);
        }
    }

    fclose(file);

    printf("Partie enregistrée avec succés dans le fichier au tour %d. !\n", state->turn);
    return 1;
}

int load_game(const char*filename, GameState *state) {
    if (state == NULL) return 0;

    // 1. Ouverture du fichier binaire ("rb" pour Read Binary)
    FILE *file = fopen(filename, "rb");
    if (file == NULL) {
        printf("Erreur Lil bro: Impossible de lire le fichier de sauvegarde. '%s' \n", filename);
        return 0;
    }

    // 2. Sécurité : Nettoyer la mémoire si on charge une partie par dessus une autre 
    cleanup_game(state);
    memset(state, 0, sizeof(GameState)); // Réinitialisation de la structure de jeu

    // 3. Lecture des variables de contexte 
    fread(&state->turn, sizeof(int), 1, file);
    fread(&state->seed, sizeof(int), 1, file);
    fread(&state->game_over, sizeof(int), 1, file);

    // 4. Lecture du joueur
    fread(&state->player, sizeof(Player), 1, file);

    // 5. Lecture des villes
    fread(&state->city_count, sizeof(int), 1, file);
    for (int i=0; i<state->city_count; i++) {
        state->cities[i] = malloc(sizeof(City));
        fread(state->cities[i], sizeof(City), 1, file);

        // Reconstruction du lien: le pointeur appartien au joueur chargé
        state->cities[i]->owner = &state->player;

    }
    // 6. Lecture des dimensions de la carte 
    fread(&state->map.width, sizeof(int), 1, file);
    fread(&state->map.height, sizeof(int), 1, file);
    fread(&state->map.start_x, sizeof(int), 1, file);
    fread(&state->map.start_y, sizeof(int), 1, file);

    // 7. Allocation et lecture des tuiles de la carte 
    state->map.tiles = malloc(state->map.height * sizeof(Tile*));
    for (int y=0; y< state->map.height; y++) {
        state->map.tiles[y] = malloc(state->map.width * sizeof(Tile));
        for (int x=0; x< state->map.width; x++) {
            // On initialise tous les pointeurs à vide par sécurité
            state->map.tiles[y][x].city = NULL;
            state->map.tiles[y][x].unit = NULL;
            state->map.tiles[y][x].camp = NULL;
            fread(&state->map.tiles[y][x], sizeof(Tile), 1, file);
            fread(&state->map.tiles[y][x].visibility, sizeof(FogState), 1, file); // Lecture de l'état de visibilité de chaque tuile
        }
    }

    // 8. Replacer les villes sur la carte (Maintenant que la carte existe !)
    for (int i=0; i< state->city_count; i++) {
        int cx = state->cities[i]->x;
        int cy = state->cities[i]->y;
        state->map.tiles[cy][cx].city = state->cities[i];
    }

    // 8bis. Lecture et reconstruction des campements barbares
    fread(&state->barbarian_camp_count, sizeof(int), 1, file);
    fread(&state->barbarian_camps_destroyed, sizeof(int), 1, file);
    if (state->barbarian_camp_count > 0) {
        state->barbarian_camps = calloc((size_t)state->barbarian_camp_count, sizeof(BarbarianCamp));
        for (int i = 0; i < state->barbarian_camp_count; ++i) {
            fread(&state->barbarian_camps[i], sizeof(BarbarianCamp), 1, file);
            if (state->barbarian_camps[i].x >= 0 && state->barbarian_camps[i].y >= 0 &&
                state->barbarian_camps[i].x < state->map.width && state->barbarian_camps[i].y < state->map.height) {
                state->map.tiles[state->barbarian_camps[i].y][state->barbarian_camps[i].x].camp = &state->barbarian_camps[i];
            }
        }
    }

    // 9. Lecture et placement des unités du joueur
    fread(&state->player_unit_count, sizeof(int), 1, file);
    for (int i=0; i<state->player_unit_count; i++) {
        state->player_units[i] = malloc(sizeof(Unit));
        fread(state->player_units[i], sizeof(Unit), 1, file);

        // Reconstruction du lien: le pointeur appartien au joueur chargé
        state->player_units[i]->owner = &state->player;
        int ux = state->player_units[i]->x;
        int uy = state->player_units[i]->y;
        state->map.tiles[uy][ux].unit = state->player_units[i];
    }

    // 10. Lecture et placement des barbares
    fread(&state->barbarian_count, sizeof(int), 1, file);
    for (int i=0; i<state->barbarian_count; i++) {
        state->barbarians[i] = malloc(sizeof(Unit));
        fread(state->barbarians[i], sizeof(Unit), 1, file);

        // Les barbares n'ont pas de lien à reconstruire (pas de propriétaire)
        int bx = state->barbarians[i]->x;
        int by = state->barbarians[i]->y;
        state->map.tiles[by][bx].unit = state->barbarians[i];
    }

    fclose(file);
    printf("Parfait Lil bro, partie chargée avec succés depuis le fichier au tour %d !\n", state->turn);
    return 1;
}

void save_final_score(const char* player_name, int score, int turn, int status) {
    FILE *f = fopen("score_history.txt", "a");  // "a" pour append (ajouter à la fin du fichier)
    if (f == NULL) {
        printf("Erreur : Impossible d'ouvrir le fichier de score. \n");
        return;
    }

    time_t t = time(NULL);
    struct tm tm = *localtime(&t);

    const char *outcome = (status == 1) ? "VICTOIRE" : "DEFAITE";

    fprintf(f, "[%02d/%02d/%04d] | Joueur: %-10s | Score: %-5d | Tours: %-3d | Issue: %s\n",
            tm.tm_mday, tm.tm_mon + 1, tm.tm_year + 1900,
            player_name, score, turn, outcome);
    fclose(f);
}

void display_history_cli() {
    FILE *f = fopen("score_history.txt", "r");
    if (f == NULL) {
        printf("Aucun historique de score trouvé. \n");
        return;
    }

    printf("\n=== TABLEAU DES SCORES ===\n");
    char line[256];
    while (fgets(line, sizeof(line), f)) {
        printf("%s", line);
    }
    fclose(f);
}

void display_history_ncurses() {
    clear(); // On nettoie l'écran ncurses
    int max_y, max_x;
    getmaxyx(stdscr, max_y, max_x);

    // On dessine une belle boîte autour
    attron(COLOR_PAIR(1));
    box(stdscr, 0, 0);
    attroff(COLOR_PAIR(1));

    // Titre
    attron(COLOR_PAIR(3) | A_BOLD);
    mvprintw(2, (max_x - 30) / 2, "=== HISTORIQUE DES SCORES ===");
    attroff(COLOR_PAIR(3) | A_BOLD);

    FILE* f = fopen("scores.txt", "r");
    if (f == NULL) {
        mvprintw(5, 5, "Aucune partie n'a encore été enregistrée.");
    } else {
        char line[256];
        int y = 5;
        // On lit le fichier et on l'affiche ligne par ligne (avec limite de l'écran)
        while (fgets(line, sizeof(line), f) && y < max_y - 3) {
            mvprintw(y, 5, "%s", line);
            y++;
        }
        fclose(f);
    }

    attron(COLOR_PAIR(4) | A_BOLD);
    mvprintw(max_y - 2, (max_x - 40) / 2, "Appuyez sur une touche pour revenir...");
    attroff(COLOR_PAIR(4) | A_BOLD);
    
    refresh();
    getch(); // On attend que le joueur appuie sur une touche
}
