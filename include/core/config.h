#ifndef CORE_CONFIG_H
#define CORE_CONFIG_H

#include <stdbool.h>

typedef enum Difficulty {
    DIFF_EASY = 0,
    DIFF_NORMAL = 1,
    DIFF_HARD = 2,
} Difficulty;

typedef struct GameConfig {
    Difficulty difficulty;
    float production_multiplier;           // multiplicateur de production (1.0 = normal)
    float growth_threshold_multiplier;     // multiplicateur du seuil de croissance
    float barbarian_spawn_multiplier;      // multiplicateur de spawn barbares
    bool famine_enabled;                    // activer/désactiver la famine
} GameConfig;

void game_config_init_default(GameConfig *cfg);
void game_config_apply_preset(GameConfig *cfg, Difficulty d);
int difficulty_from_string(const char *value, Difficulty *out);
int config_parse_float(const char *value, float *out);
int config_parse_bool(const char *value, bool *out);
const char *difficulty_to_string(Difficulty d);

#endif // CORE_CONFIG_H
