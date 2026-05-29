#include <stdio.h>
#include <stdbool.h>
#include "../include/core/types.h"
#include "../include/core/tech.h"
#include "../include/core/game.h"

// Variable globale statique pour compter les erreurs dans ce fichier
static int tech_errors = 0;

// Macro d'assertion personnalisée avec des messages très précis
#define ASSERT_TECH(expr, msg) do { \
    printf("    -> Test: %s... ", msg); \
    if (expr) { \
        printf("\033[1;32mOK\033[0m\n"); \
    } else { \
        tech_errors++; \
        printf("\033[1;31mECHEC\033[0m (Ligne %d)\n", __LINE__); \
    } \
} while(0)

// Fonction exposée pour lancer la suite de tests
void run_tech_tests(void) {
    printf("\n=======================================================\n");
    printf("[SUITE DE TESTS] : Moteur Technologique (tech.c)\n");
    printf("=======================================================\n");

    tech_errors = 0;
    Player p;
    
    // Initialisation d'un joueur vierge
    for (int i = 0; i < MAX_TECHNOLOGIES; i++) {
        p.unlocked_techs[i] = false;
    }
    p.science = 0;
    p.current_research = TECH_NONE;

    printf("\n--- SCENARIO 1 : Vérification des prérequis de départ ---\n");
    ASSERT_TECH(can_research(&p, TECH_CHASSE) == true, "La Chasse doit etre debloquee par defaut");
    ASSERT_TECH(can_research(&p, TECH_AGRICULTURE) == true, "L'Agriculture doit etre debloquee par defaut");
    ASSERT_TECH(can_research(&p, TECH_MAÇONNERIE) == false, "La Maconnerie doit etre VERROUILLEE (requiert Artisanat)");
    ASSERT_TECH(can_research(&p, TECH_BATEAU) == false, "Les Bateaux doivent etre VERROUILLES (requiert Commerce)");

    printf("\n--- SCENARIO 2 : Déblocage en cascade après recherche ---\n");
    // Simulation : Le joueur a recherché l'Artisanat
    p.unlocked_techs[TECH_ARTISANAT] = true;
    ASSERT_TECH(can_research(&p, TECH_MAÇONNERIE) == true, "La Maconnerie doit se debloquer suite a l'Artisanat");
    ASSERT_TECH(can_research(&p, TECH_COMMERCE) == false, "Le Commerce doit rester bloqué(il manque l'écriture)");
    ASSERT_TECH(can_research(&p, TECH_ARTISANAT) == false, "Une technologie deja acquise ne doit plus etre proposable");
    // Simulation : Le joueur recherche ensuite l'Écriture
    p.unlocked_techs[TECH_ECRITURE] = true;
    ASSERT_TECH(can_research(&p, TECH_COMMERCE) == true, "Le Commerce doit se debloquer (Artisanat + Ecriture acquis)");
    // -------------------------------------------------

    ASSERT_TECH(can_research(&p, TECH_ARTISANAT) == false, "Une technologie deja acquise ne doit plus etre proposable");

    printf("\n--- SCENARIO 3 : Mécanique de la file de recherche ---\n");
    start_research(&p, TECH_AGRICULTURE);
    ASSERT_TECH(p.current_research == TECH_AGRICULTURE, "Le projet actif du joueur doit etre mis a jour sur Agriculture");

    if (tech_errors == 0) {
        printf("\n\033[1;32m-> Bilan tech.c : 100%% SUCCES\033[0m\n");
    } else {
        printf("\n\033[1;31m-> Bilan tech.c : %d ERREUR(S) DETECTEE(S)\033[0m\n", tech_errors);
    }
}