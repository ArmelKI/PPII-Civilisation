#include "../../include/core/config.h"
#include <stdio.h>
#include <string.h>
#include <stdlib.h>

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
