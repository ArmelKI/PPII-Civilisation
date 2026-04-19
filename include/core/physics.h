#ifndef CORE_PHYSICS_H
#define CORE_PHYSICS_H

#include "core/types.h"

int get_terrain_pm_cost(TerrainType type);
int can_move(Unit *unit, Tile *destination);
void resolve_combat(Unit *attacker, Unit *defender);

#endif // CORE_PHYSICS_H