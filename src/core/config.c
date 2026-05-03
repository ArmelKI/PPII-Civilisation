#include "../../include/core/config.h"
#include <stdio.h>

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
