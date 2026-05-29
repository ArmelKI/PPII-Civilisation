#include <stdio.h>
#include <stdlib.h>

// On garde la variable globale des erreurs ici
int g_failed = 0;

int main(void)
{
    // 1. DÉCLARATION DE TOUS LES TESTS EXTERNES
    extern int run_test_physique(void);
    extern int run_test_config(void);
    extern int run_test_archer(void);
    extern int run_test_cavalier(void);
    extern int run_test_difficulty_effects(void);
    
    // La nouvelle fonction qui regroupe les tests d'Armel :
    extern int run_test_barbares(void); 
    
    // Nos tests
    extern void run_tech_tests(void);
    extern void run_save_tests(void);

    extern int run_test_ressources(void);
    extern int run_test_special_resources(void);
    extern int run_test_wonders(void);

    // 2. EXÉCUTION
    printf("\n=== LANCEMENT DE LA SUITE DE TESTS GLOBALE ===\n\n");

    // Lancement des tests existants
    run_test_physique();
    run_test_config();
    if (run_test_archer() != 0) g_failed++;
    if (run_test_cavalier() != 0) g_failed++;
    if (run_test_difficulty_effects() != 0) g_failed++;

    // Lancement du module barbares complet
    if (run_test_barbares() != 0) g_failed++;

    // Lancement de nos tests
    run_tech_tests();
    run_save_tests();

    if (run_test_ressources() != 0) g_failed++;
    if (run_test_special_resources() != 0) g_failed++;
    if (run_test_wonders() != 0) g_failed++;

    // 3. BILAN FINAL
    printf("\n=======================================================\n");
    if (g_failed == 0) {
        printf("\033[1;32m[SUCCES TOTAL] Tous les modules sont operationnels.\033[0m\n");
        return 0;
    }

    fprintf(stderr, "\033[1;31m[ECHEC] %d test(s) ont echoue.\033[0m\n", g_failed);
    return 1;
}