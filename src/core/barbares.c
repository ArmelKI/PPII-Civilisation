#include <stdlib.h>
#include <stdio.h>
#include <math.h>
#include <limits.h>

#include "core/barbares.h"
#include "core/types.h"

typedef struct {
    int x;
    int y;
    int kind;      // 0 = unité, 1 = ville
    int priority;
    int dist;
} TargetCandidate;

static int chebyshev_distance(int x1, int y1, int x2, int y2)
{
    int dx = abs(x2 - x1);
    int dy = abs(y2 - y1);
    return (dx > dy) ? dx : dy;
}

static int terrain_move_cost(TerrainType type)
{
    if (type == TERRAIN_FORET) {
        return 2;
    }
    if (type == TERRAIN_MONTAGNE) {
        return 3;
    }
    return 1;
}

static int is_inside(const Map *map, int x, int y)
{
    return map && x >= 0 && x < map->width && y >= 0 && y < map->height;
}

static int is_walkable_for_barbarian(const Map *map, int x, int y, const Unit *barb)
{
    if (!is_inside(map, x, y)) {
        return 0;
    }

    const Tile *tile = &map->tiles[y][x];
    if (tile->type == TERRAIN_EAU) {
        return 0;
    }

    if (tile->city) {
        return 0;
    }

    if (tile->unit && !(barb && barb->x == x && barb->y == y)) {
        return 0;
    }

    return 1;
}

static int candidate_cmp(const TargetCandidate *a, const TargetCandidate *b)
{
    if (a->priority != b->priority) {
        return (a->priority > b->priority) ? -1 : 1;
    }
    if (a->dist != b->dist) {
        return (a->dist < b->dist) ? -1 : 1;
    }
    return 0;
}

// Trouve un premier pas vers une case adjacente à la cible en évitant les obstacles.
static int find_step_towards_target(const Unit *barb, const Map *map, int target_x, int target_y,
                                    int *out_x, int *out_y)
{
    if (!barb || !map || !out_x || !out_y) {
        return 0;
    }

    const int w = map->width;
    const int h = map->height;
    const int size = w * h;
    const int start = barb->y * w + barb->x;

    int *parent = (int *)malloc((size_t)size * sizeof(int));
    unsigned char *visited = (unsigned char *)calloc((size_t)size, sizeof(unsigned char));
    unsigned char *goals = (unsigned char *)calloc((size_t)size, sizeof(unsigned char));
    int *queue = (int *)malloc((size_t)size * sizeof(int));

    if (!parent || !visited || !goals || !queue) {
        free(parent);
        free(visited);
        free(goals);
        free(queue);
        return 0;
    }

    for (int i = 0; i < size; ++i) {
        parent[i] = -1;
    }

    // Objectif: une case adjacente libre à la cible, pas la case cible elle-même.
    for (int dy = -1; dy <= 1; ++dy) {
        for (int dx = -1; dx <= 1; ++dx) {
            if (dx == 0 && dy == 0) {
                continue;
            }
            int gx = target_x + dx;
            int gy = target_y + dy;
            if (!is_walkable_for_barbarian(map, gx, gy, barb)) {
                continue;
            }
            goals[gy * w + gx] = 1;
        }
    }

    if (goals[start]) {
        free(parent);
        free(visited);
        free(goals);
        free(queue);
        return 0;
    }

    int q_head = 0;
    int q_tail = 0;
    queue[q_tail++] = start;
    visited[start] = 1;

    int found = -1;
    while (q_head < q_tail) {
        int cur = queue[q_head++];
        if (goals[cur]) {
            found = cur;
            break;
        }

        int cx = cur % w;
        int cy = cur / w;

        for (int dy = -1; dy <= 1; ++dy) {
            for (int dx = -1; dx <= 1; ++dx) {
                if (dx == 0 && dy == 0) {
                    continue;
                }

                int nx = cx + dx;
                int ny = cy + dy;
                if (!is_walkable_for_barbarian(map, nx, ny, barb)) {
                    continue;
                }

                int next_idx = ny * w + nx;
                if (visited[next_idx]) {
                    continue;
                }

                visited[next_idx] = 1;
                parent[next_idx] = cur;
                queue[q_tail++] = next_idx;
            }
        }
    }

    if (found < 0) {
        free(parent);
        free(visited);
        free(goals);
        free(queue);
        return 0;
    }

    int step = found;
    while (parent[step] != -1 && parent[step] != start) {
        step = parent[step];
    }

    if (parent[step] == -1) {
        free(parent);
        free(visited);
        free(goals);
        free(queue);
        return 0;
    }

    *out_x = step % w;
    *out_y = step / w;

    free(parent);
    free(visited);
    free(goals);
    free(queue);
    return 1;
}

static int patrol_random_step(Unit *barb, Map *map)
{
    int options_x[8];
    int options_y[8];
    int count = 0;

    for (int dy = -1; dy <= 1; ++dy) {
        for (int dx = -1; dx <= 1; ++dx) {
            if (dx == 0 && dy == 0) {
                continue;
            }

            int nx = barb->x + dx;
            int ny = barb->y + dy;
            if (!is_walkable_for_barbarian(map, nx, ny, barb)) {
                continue;
            }

            int cost = terrain_move_cost(map->tiles[ny][nx].type);
            if (barb->pm < cost) {
                continue;
            }

            options_x[count] = nx;
            options_y[count] = ny;
            count++;
        }
    }

    if (count == 0) {
        return 0;
    }

    int pick = rand() % count;
    int nx = options_x[pick];
    int ny = options_y[pick];
    int cost = terrain_move_cost(map->tiles[ny][nx].type);

    map->tiles[barb->y][barb->x].unit = NULL;
    barb->x = nx;
    barb->y = ny;
    barb->pm -= cost;
    map->tiles[barb->y][barb->x].unit = barb;
    return 1;
}

// Place les campements barbares au démarrage
BarbarianCamp *barbares_init_camps(Map *map, int num_camps, int player_city_x, int player_city_y)
{
    // On commence par vérifier les entrées pour éviter les crashes.
    if (num_camps <= 0 || !map) {
        return NULL;
    }

    BarbarianCamp *camps = (BarbarianCamp *)malloc(num_camps * sizeof(BarbarianCamp));
    if (!camps) {
        fprintf(stderr, "[barbares] Erreur d'allocation des campements\n");
        return NULL;
    }

    for (int i = 0; i < num_camps; ++i) {
        camps[i].id = i;
        camps[i].destroyed = 1;
        camps[i].x = -1;
        camps[i].y = -1;
    }

    // Pour chaque campement, on tente un placement aléatoire.
    // Si ça ne marche pas, on fait un balayage complet de la carte.
    for (int i = 0; i < num_camps; ++i) {
        int placed = 0;

        for (int attempt = 0; attempt < 500 && !placed; ++attempt) {
            int x = rand() % map->width;
            int y = rand() % map->height;

            if (map->tiles[y][x].type == TERRAIN_EAU) {
                continue;
            }

            if (map->tiles[y][x].city || map->tiles[y][x].unit) {
                continue;
            }

            if (abs(x - player_city_x) < 5 && abs(y - player_city_y) < 5) {
                continue;
            }

            camps[i].x = x;
            camps[i].y = y;
            camps[i].destroyed = 0;
            placed = 1;
        }

        if (!placed) {
            // Solution de secours : on parcourt toute la carte.
            for (int y = 0; y < map->height && !placed; ++y) {
                for (int x = 0; x < map->width && !placed; ++x) {
                    if (map->tiles[y][x].type == TERRAIN_EAU) {
                        continue;
                    }

                    if (map->tiles[y][x].city || map->tiles[y][x].unit) {
                        continue;
                    }

                    if (abs(x - player_city_x) < 5 && abs(y - player_city_y) < 5) {
                        continue;
                    }

                    camps[i].x = x;
                    camps[i].y = y;
                    camps[i].destroyed = 0;
                    placed = 1;
                }
            }
        }

        if (!placed) {
            free(camps);
            return NULL;
        }
    }

    return camps;
}

// Génère une unité barbare chaque tour (plafond N*3)
int barbares_spawn_unit(BarbarianCamp *camp, Map *map, Unit **units_array, int *num_units, int max_units)
{
    // On refuse tout de suite si les paramètres ne sont pas cohérents.
    if (!camp || camp->destroyed || !map || !units_array || !num_units) {
        return 0;
    }

    // Le sujet impose un plafond global sur les barbares vivants.
        if (*num_units >= max_units) {
            return 0;
        }

    // Si le camp n'a pas de position valide, on ne tente rien.
        if (camp->x < 0 || camp->y < 0) {
            return 0;
        }

    // Si la case du camp est déjà occupée, on attend le tour suivant.
        if (map->tiles[camp->y][camp->x].unit) {
            return 0;
        }

    // Création d'un guerrier barbare simple.
        Unit *barb = (Unit *)malloc(sizeof(Unit));
        if (!barb) {
            return 0;
        }

        barb->type = UNIT_BARBARIAN;
        barb->hp = 15;
        barb->hp_max = 15;
        barb->attack = 3;
        barb->defense = 2;
        barb->pm = 3;
        barb->pm_max = 3;
        barb->x = camp->x;
        barb->y = camp->y;
        barb->owner = NULL;

        units_array[*num_units] = barb;
        (*num_units)++;
        map->tiles[barb->y][barb->x].unit = barb;

        return 1;
}

// Pathfinding simple vers cible la plus proche
void barbares_move(Unit *barb, Map *map, City *player_cities[], int num_cities, 
                   Unit *player_units[], int num_player_units)
{
    if (!barb || !map || barb->hp <= 0 || barb->pm <= 0) {
        return;
    }

    TargetCandidate candidates[128];
    int candidate_count = 0;

    // Priorité IA: colon > guerrier > ville.
    // On garde aussi la distance pour ne pas partir à l'opposé sans raison.
    for (int i = 0; i < num_player_units; ++i) {
        Unit *u = player_units[i];
        if (!u || u->hp <= 0) {
            continue;
        }

        if (candidate_count >= (int)(sizeof(candidates) / sizeof(candidates[0]))) {
            break;
        }

        int priority = (u->type == UNIT_SETTLER) ? 300 : 200;
        if (u->hp <= (u->hp_max / 2)) {
            priority += 20;
        }

        candidates[candidate_count].x = u->x;
        candidates[candidate_count].y = u->y;
        candidates[candidate_count].kind = 0;
        candidates[candidate_count].priority = priority;
        candidates[candidate_count].dist = chebyshev_distance(barb->x, barb->y, u->x, u->y);
        candidate_count++;
    }

    for (int i = 0; i < num_cities; ++i) {
        City *c = player_cities[i];
        if (!c || c->hp <= 0) {
            continue;
        }

        if (candidate_count >= (int)(sizeof(candidates) / sizeof(candidates[0]))) {
            break;
        }

        candidates[candidate_count].x = c->x;
        candidates[candidate_count].y = c->y;
        candidates[candidate_count].kind = 1;
        candidates[candidate_count].priority = 100;
        candidates[candidate_count].dist = chebyshev_distance(barb->x, barb->y, c->x, c->y);
        candidate_count++;
    }

    // Tri simple pour garder un code compact et lisible.
    for (int i = 0; i < candidate_count; ++i) {
        int best = i;
        for (int j = i + 1; j < candidate_count; ++j) {
            if (candidate_cmp(&candidates[j], &candidates[best]) < 0) {
                best = j;
            }
        }
        if (best != i) {
            TargetCandidate tmp = candidates[i];
            candidates[i] = candidates[best];
            candidates[best] = tmp;
        }
    }

    for (int i = 0; i < candidate_count; ++i) {
        int new_x = -1;
        int new_y = -1;
        if (!find_step_towards_target(barb, map, candidates[i].x, candidates[i].y, &new_x, &new_y)) {
            continue;
        }

        int cost = terrain_move_cost(map->tiles[new_y][new_x].type);
        if (barb->pm < cost) {
            continue;
        }

        map->tiles[barb->y][barb->x].unit = NULL;
        barb->x = new_x;
        barb->y = new_y;
        barb->pm -= cost;
        map->tiles[barb->y][barb->x].unit = barb;
        return;
    }

    // Aucune cible atteignable: patrouille aléatoire contrôlée plutôt que blocage complet.
    (void)patrol_random_step(barb, map);
}

// Combat automatique si adjacent
int barbares_attack_if_adjacent(Unit *barb, Map *map, City *player_cities[], int num_cities,
                                 Unit *player_units[], int num_player_units)
{
    if (!barb || !map) {
        return 0;
    }

    // Ordre deterministe : on essaie d'abord les unites du joueur, puis les villes.
    for (int i = 0; i < num_player_units; ++i) {
        Unit *target = player_units[i];
        if (!target || target->hp <= 0) {
            continue;
        }

        int dx = abs(target->x - barb->x);
        int dy = abs(target->y - barb->y);
        if (dx > 1 || dy > 1 || (dx == 0 && dy == 0)) {
            continue;
        }

        int dmg_to_target = barb->attack - target->defense;
        if (dmg_to_target < 1) {
            dmg_to_target = 1;
        }

        int dmg_to_barb = target->attack - barb->defense;
        if (dmg_to_barb < 0) {
            dmg_to_barb = 0;
        }

        target->hp -= dmg_to_target;
        barb->hp -= dmg_to_barb;

        if (target->hp <= 0) {
            target->hp = 0;
            if (target->x >= 0 && target->y >= 0) {
                map->tiles[target->y][target->x].unit = NULL;
            }
            player_units[i] = NULL;
        }

        if (barb->hp <= 0) {
            barb->hp = 0;
            if (barb->x >= 0 && barb->y >= 0) {
                map->tiles[barb->y][barb->x].unit = NULL;
            }
        }

        return 1;
    }

    for (int i = 0; i < num_cities; ++i) {
        City *city = player_cities[i];
        if (!city || city->hp <= 0) {
            continue;
        }

        int dx = abs(city->x - barb->x);
        int dy = abs(city->y - barb->y);
        if (dx > 1 || dy > 1 || (dx == 0 && dy == 0)) {
            continue;
        }

        int city_power = city->has_wall ? 30 : 20;
        int dmg_to_city = barb->attack - city_power;
        if (dmg_to_city < 1) {
            dmg_to_city = 1;
        }

        int dmg_to_barb = city_power - barb->defense;
        if (dmg_to_barb < 0) {
            dmg_to_barb = 0;
        }

        city->hp -= dmg_to_city;
        barb->hp -= dmg_to_barb;

        if (city->hp <= 0) {
            city->hp = 0;
        }

        if (barb->hp <= 0) {
            barb->hp = 0;
            if (barb->x >= 0 && barb->y >= 0) {
                map->tiles[barb->y][barb->x].unit = NULL;
            }
        }

        return 1;
    }

    return 0;
}

// Détruit un campement et donne bonus or
void barbares_destroy_camp(BarbarianCamp *camp, int *total_destroyed, Player *player)
{
    if (!camp || camp->destroyed) {
        return;
    }

    camp->destroyed = 1;

    if (total_destroyed) {
        (*total_destroyed)++;
    }

    if (player && total_destroyed) {
        player->gold += 5 * (*total_destroyed);
    }
}

// Libère la mémoire des campements
void barbares_cleanup(BarbarianCamp *camps, int num_camps)
{
    (void)num_camps;

    if (camps) {
        free(camps);
    }
}
