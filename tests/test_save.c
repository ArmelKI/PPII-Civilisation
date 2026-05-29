#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <string.h>
#include "../include/core/types.h"
#include "../include/core/game.h"
#include "../include/core/save.h"

// Variable globale statique pour compter les erreurs dans ce fichier
static int save_errors = 0;

#define ASSERT_SAVE(expr, msg) do { \
    printf("    -> Test: %s... ", msg); \
    if (expr) { \
        printf("\033[1;32mOK\033[0m\n"); \
    } else { \
        save_errors++; \
        printf("\033[1;31mECHEC\033[0m (Ligne %d)\n", __LINE__); \
    } \
} while(0)

void run_save_tests(void) {
    printf("\n=======================================================\n");
    printf("[SUITE DE TESTS] : Systeme de Sauvegarde (save.c)\n");
    printf("=======================================================\n");

    save_errors = 0;
    GameState game_src;
    GameState game_dest;

    memset(&game_src, 0, sizeof(GameState));
    memset(&game_dest, 0, sizeof(GameState));

    printf("\n--- SCENARIO 1 : Préparation d'une sauvegarde factice ---\n");
    // On peuple notre partie témoin avec des valeurs très spécifiques
    game_src.turn = 42;
    game_src.max_turns = 200;
    game_src.player.gold = 1337;
    game_src.player.science = 88;
    game_src.player.current_research = TECH_ECRITURE;
    game_src.player.unlocked_techs[TECH_AGRICULTURE] = true;
    game_src.city_count = 0;
    game_src.player_unit_count = 0;
    game_src.map.width = 10;
    game_src.map.height = 10;
    
    // Allocation minimale pour éviter les crashs de sérialisation
    game_src.map.tiles = (Tile**)malloc(10 * sizeof(Tile*));
    for(int i = 0; i < 10; i++) {
        game_src.map.tiles[i] = (Tile*)calloc(10, sizeof(Tile));
        game_src.map.tiles[i][0].type = TERRAIN_PLAINE;
    }

    printf("    -> Simulation des donnees en memoire... \033[1;32mOK\033[0m\n");

    printf("\n--- SCENARIO 2 : Écriture sur le disque (Sérialisation) ---\n");
    bool save_ok = save_game("test_soutenance.tmp", &game_src);
    ASSERT_SAVE(save_ok == true, "Creation du fichier 'test_soutenance.tmp' sur le disque");

    printf("\n--- SCENARIO 3 : Relecture depuis le disque (Désérialisation) ---\n");
    bool load_ok = load_game("test_soutenance.tmp", &game_dest);
    ASSERT_SAVE(load_ok == true, "Ouverture et parsing du fichier 'test_soutenance.tmp'");

    printf("\n--- SCENARIO 4 : Vérification de l'intégrité des données ---\n");
    ASSERT_SAVE(game_dest.turn == 42, "Le numero de tour doit etre restaure a 42");
    ASSERT_SAVE(game_dest.player.gold == 1337, "Le tresor (1337 or) doit etre restaure");
    ASSERT_SAVE(game_dest.player.science == 88, "La cagnotte de science (88) doit etre restauree");
    ASSERT_SAVE(game_dest.player.current_research == TECH_ECRITURE, "Le projet de recherche actif doit etre restaure");
    ASSERT_SAVE(game_dest.player.unlocked_techs[TECH_AGRICULTURE] == true, "Le tableau booléen des technos acquises doit etre exact");

    // Nettoyage de la mémoire et suppression du fichier temporaire
    for(int i = 0; i < 10; i++) free(game_src.map.tiles[i]);
    free(game_src.map.tiles);
    
    if (game_dest.map.tiles) {
        for(int i = 0; i < game_dest.map.height; i++) {
            if (game_dest.map.tiles[i]) free(game_dest.map.tiles[i]);
        }
        free(game_dest.map.tiles);
    }
    remove("test_soutenance.tmp");

    if (save_errors == 0) {
        printf("\n\033[1;32m-> Bilan save.c : 100%% SUCCES (Aucune corruption mémoire)\033[0m\n");
    } else {
        printf("\n\033[1;31m-> Bilan save.c : %d ERREUR(S) DE CORRUPTION DETECTEE(S)\033[0m\n", save_errors);
    }
}