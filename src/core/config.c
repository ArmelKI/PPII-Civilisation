#include "../../include/core/config.h"
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <locale.h>

static GameConfig g_active_config = {
    .difficulty = DIFF_NORMAL,
    .production_multiplier = 1.0f,
    .growth_threshold_multiplier = 1.0f,
    .barbarian_spawn_multiplier = 1.0f,
    .famine_enabled = true,
};

static int parse_full_float(const char *value, float *out)
{
    double parsed_value = 0.0;
    double fraction_scale = 0.1;
    int sign = 1;
    int seen_digit = 0;
    int seen_decimal = 0;
    size_t index = 0;

    if (!value || !out) {
        return 0;
    }

    if (value[index] == '+' || value[index] == '-') {
        if (value[index] == '-') {
            sign = -1;
        }
        index++;
    }

    for (; value[index] != '\0'; ++index) {
        char c = value[index];

        if (c >= '0' && c <= '9') {
            seen_digit = 1;
            if (!seen_decimal) {
                parsed_value = (parsed_value * 10.0) + (double)(c - '0');
            } else {
                parsed_value += (double)(c - '0') * fraction_scale;
                fraction_scale *= 0.1;
            }
            continue;
        }

        if ((c == '.' || c == ',') && !seen_decimal) {
            seen_decimal = 1;
            continue;
        }

        return 0;
    }

    if (!seen_digit) {
        return 0;
    }

    *out = (float)(sign * parsed_value);
    return 1;
}

static int clamp_int(int value, int min_value, int max_value)
{
    if (value < min_value) {
        return min_value;
    }
    if (value > max_value) {
        return max_value;
    }
    return value;
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

UnitType game_config_select_barbarian_type(int roll_0_99)
{
    const GameConfig *cfg = game_config_get_active();
    float multiplier = cfg ? cfg->barbarian_spawn_multiplier : 1.0f;
    int advanced_chance = clamp_int((int)(20.0f * multiplier + 0.5f), 5, 60);
    int cavalry_chance = clamp_int(advanced_chance / 3, 1, advanced_chance - 1);

    if (roll_0_99 < cavalry_chance) {
        return UNIT_CAVALIER;
    }

    if (roll_0_99 < advanced_chance) {
        return UNIT_ARCHER;
    }

    return UNIT_BARBARIAN;
}

void game_config_apply_barbarian_stats(Unit *barbarian)
{
    if (!barbarian) {
        return;
    }

    const GameConfig *cfg = game_config_get_active();

    //sert à rendre les barbares plus fort en mode Difficile
    float multiplier = cfg ? cfg->barbarian_spawn_multiplier : 1.0f;

    int base_hp = 8;
    int base_attack = 2;
    int base_defense = 1;
    int base_pm = 1;

    if (barbarian->type == UNIT_ARCHER) {
        base_hp = 5;    //Archer très vulnerable
        base_attack = 4;    //boosté offensivement
        base_defense = 1;   //très faible en défense
        base_pm = 2;
    } else if (barbarian->type == UNIT_CAVALIER) {
        base_hp = 12;   //Cavalier plus robuste
        base_attack = 3;   //attaque similaire au guerrier classique
        base_defense = 3;  
        base_pm = 4;
    }

    barbarian->hp = (int)(base_hp * multiplier + 0.5f);
    barbarian->hp_max = barbarian->hp;
    if (barbarian->hp < 1) {
        barbarian->hp = 1;
        barbarian->hp_max = 1;
    }

    barbarian->attack = (int)(base_attack * multiplier + 0.5f);
    if (barbarian->attack < 1) {
        barbarian->attack = 1;
    }

    barbarian->defense = base_defense;

    barbarian->pm = (int)(base_pm * multiplier + 0.5f);
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
