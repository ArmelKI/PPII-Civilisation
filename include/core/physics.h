#ifndef CORE_PHYSICS_H
#define CORE_PHYSICS_H

#include "core/types.h"

int get_terrain_pm_cost(TerrainType type);
int can_move(Unit *unit, Tile *destination, Player *player);
void resolve_combat(Unit *attacker, Unit *defender);
void resolve_combat_city(Unit *attacker, City *city);

#endif // CORE_PHYSICS_H