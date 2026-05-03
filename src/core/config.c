#include "../../include/core/config.h"
#include <stdio.h>
#include <string.h>
#include <stdlib.h>

static GameConfig g_active_config = {
    .difficulty = DIFF_NORMAL,
    .production_multiplier = 1.0f,
    .growth_threshold_multiplier = 1.0f,
    .barbarian_spawn_multiplier = 1.0f,
    .famine_enabled = true,
};

static int parse_full_float(const char *value, float *out)
{
    char *end = NULL;
    float parsed_value;

    if (!value || !out) {
        return 0;
    }

    parsed_value = strtof(value, &end);
    if (end == value || *end != '\0') {
        return 0;
    }

    *out = parsed_value;
    return 1;
}

void game_config_init_default(GameConfig *cfg)
{
    if (!cfg) return;
    game_config_apply_preset(cfg, DIFF_NORMAL);
}

void game_config_apply_preset(GameConfig *cfg, Difficulty d)
{
    if (!cfg) return;
    cfg->difficulty = d;
    switch (d) {
        case DIFF_EASY:
            cfg->production_multiplier = 1.25f;
            cfg->growth_threshold_multiplier = 0.75f;
            cfg->barbarian_spawn_multiplier = 0.75f;
            cfg->famine_enabled = false;
            break;

        case DIFF_HARD:
            cfg->production_multiplier = 0.75f;
            cfg->growth_threshold_multiplier = 1.25f;
            cfg->barbarian_spawn_multiplier = 1.25f;
            cfg->famine_enabled = true;
            break;

        case DIFF_NORMAL:
        default:
            cfg->production_multiplier = 1.0f;
            cfg->growth_threshold_multiplier = 1.0f;
            cfg->barbarian_spawn_multiplier = 1.0f;
            cfg->famine_enabled = true;
            break;
    }
}

void game_config_set_active(const GameConfig *cfg)
{
    if (!cfg) {
        game_config_init_default(&g_active_config);
        return;
    }

    g_active_config = *cfg;
}

const GameConfig *game_config_get_active(void)
{
    return &g_active_config;
}

int game_config_barbarian_spawn_chance(void)
{
    const GameConfig *cfg = game_config_get_active();
    float multiplier = cfg ? cfg->barbarian_spawn_multiplier : 1.0f;
    int chance = (int)(35.0f * multiplier + 0.5f);

    if (chance < 5) {
        chance = 5;
    } else if (chance > 95) {
        chance = 95;
    }

    return chance;
}

void game_config_apply_barbarian_stats(Unit *barbarian)
{
    if (!barbarian) {
        return;
    }

    const GameConfig *cfg = game_config_get_active();
    float multiplier = cfg ? cfg->barbarian_spawn_multiplier : 1.0f;

    barbarian->hp = (int)(15.0f * multiplier + 0.5f);
    barbarian->hp_max = barbarian->hp;
    if (barbarian->hp < 1) {
        barbarian->hp = 1;
        barbarian->hp_max = 1;
    }

    barbarian->attack = (int)(3.0f * multiplier + 0.5f);
    if (barbarian->attack < 1) {
        barbarian->attack = 1;
    }

    barbarian->pm = (int)(3.0f * multiplier + 0.5f);
    barbarian->pm_max = barbarian->pm;
    if (barbarian->pm < 1) {
        barbarian->pm = 1;
        barbarian->pm_max = 1;
    }
}

const char *difficulty_to_string(Difficulty d)
{
    switch (d) {
        case DIFF_EASY: return "Facile";
        case DIFF_HARD: return "Difficile";
        case DIFF_NORMAL:
        default:
            return "Normal";
    }
}

int difficulty_from_string(const char *value, Difficulty *out)
{
    if (!value || !out) {
        return 0;
    }

    if (strcmp(value, "facile") == 0 || strcmp(value, "easy") == 0) {
        *out = DIFF_EASY;
        return 1;
    }

    if (strcmp(value, "normal") == 0 || strcmp(value, "moyen") == 0) {
        *out = DIFF_NORMAL;
        return 1;
    }

    if (strcmp(value, "difficile") == 0 || strcmp(value, "hard") == 0) {
        *out = DIFF_HARD;
        return 1;
    }

    return 0;
}

int config_parse_float(const char *value, float *out)
{
    return parse_full_float(value, out);
}

int config_parse_bool(const char *value, bool *out)
{
    if (!value || !out) {
        return 0;
    }

    if (strcmp(value, "1") == 0 || strcmp(value, "true") == 0 || strcmp(value, "vrai") == 0 || strcmp(value, "yes") == 0 || strcmp(value, "on") == 0 || strcmp(value, "active") == 0) {
        *out = true;
        return 1;
    }

    if (strcmp(value, "0") == 0 || strcmp(value, "false") == 0 || strcmp(value, "faux") == 0 || strcmp(value, "no") == 0 || strcmp(value, "off") == 0 || strcmp(value, "desactive") == 0) {
        *out = false;
        return 1;
    }

    return 0;
}
