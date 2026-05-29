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

static void test_parse_difficulty(void)
{
    Difficulty diff = DIFF_NORMAL;

    EXPECT(difficulty_from_string("facile", &diff) == 1, "parse facile");
    EXPECT(diff == DIFF_EASY, "facile should map to DIFF_EASY");

    EXPECT(difficulty_from_string("normal", &diff) == 1, "parse normal");
    EXPECT(diff == DIFF_NORMAL, "normal should map to DIFF_NORMAL");

    EXPECT(difficulty_from_string("difficile", &diff) == 1, "parse difficile");
    EXPECT(diff == DIFF_HARD, "difficile should map to DIFF_HARD");

    EXPECT(difficulty_from_string("invalid", &diff) == 0, "invalid difficulty should be rejected");
}

static void test_parse_overrides(void)
{
    float value = 0.0f;
    bool enabled = false;

    EXPECT(config_parse_float("1.50", &value) == 1, "parse float override");
    EXPECT(fabsf(value - 1.5f) < 1e-6f, "parsed float value should match");

    EXPECT(config_parse_float("abc", &value) == 0, "invalid float should be rejected");

    EXPECT(config_parse_bool("true", &enabled) == 1, "parse true");
    EXPECT(enabled == true, "true should map to true");

    EXPECT(config_parse_bool("off", &enabled) == 1, "parse off");
    EXPECT(enabled == false, "off should map to false");

    EXPECT(config_parse_bool("maybe", &enabled) == 0, "invalid bool should be rejected");
}

static void test_difficulty_labels(void)
{
    EXPECT(strcmp(difficulty_to_string(DIFF_EASY), "Facile") == 0, "easy label should be Facile");
    EXPECT(strcmp(difficulty_to_string(DIFF_NORMAL), "Normal") == 0, "normal label should be Normal");
    EXPECT(strcmp(difficulty_to_string(DIFF_HARD), "Difficile") == 0, "hard label should be Difficile");
}

int run_test_config(void)
{
    test_default_preset();
    test_presets_values();
    test_parse_difficulty();
    test_parse_overrides();
    test_difficulty_labels();

    if (g_failed == 0) {
        puts("[OK] Config tests passed.");
        return 0;
    }

    fprintf(stderr, "[FAIL] %d test(s) failed.\n", g_failed);
    return 1;
}
