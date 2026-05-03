#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>

#include "core/config.h"

static int g_failed = 0;

#define EXPECT(cond, msg)                                                       \
    do {                                                                        \
        if (!(cond)) {                                                          \
            fprintf(stderr, "[FAIL] %s\n", (msg));                          \
            g_failed++;                                                         \
        }                                                                       \
    } while (0)

static void test_default_preset(void)
{
    GameConfig cfg;
    game_config_init_default(&cfg);

    EXPECT(cfg.difficulty == DIFF_NORMAL, "default difficulty should be Normal");
    EXPECT(fabsf(cfg.production_multiplier - 1.0f) < 1e-6f, "production multiplier default");
    EXPECT(fabsf(cfg.growth_threshold_multiplier - 1.0f) < 1e-6f, "growth threshold multiplier default");
    EXPECT(cfg.famine_enabled == true, "famine should be enabled by default");
}

static void test_presets_values(void)
{
    GameConfig cfg;

    game_config_apply_preset(&cfg, DIFF_EASY);
    EXPECT(cfg.difficulty == DIFF_EASY, "easy preset difficulty");
    EXPECT(fabsf(cfg.production_multiplier - 1.25f) < 1e-6f, "easy production multiplier");
    EXPECT(cfg.famine_enabled == false, "easy should disable famine");

    game_config_apply_preset(&cfg, DIFF_HARD);
    EXPECT(cfg.difficulty == DIFF_HARD, "hard preset difficulty");
    EXPECT(fabsf(cfg.production_multiplier - 0.75f) < 1e-6f, "hard production multiplier");
    EXPECT(cfg.famine_enabled == true, "hard should enable famine");
}

int run_test_config(void)
{
    test_default_preset();
    test_presets_values();

    if (g_failed == 0) {
        puts("[OK] Config tests passed.");
        return 0;
    }

    fprintf(stderr, "[FAIL] %d test(s) failed.\n", g_failed);
    return 1;
}
