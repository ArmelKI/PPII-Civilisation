#ifndef TECH_H
#define TECH_H

#include "types.h"



int can_research(Player *p, TechType tech);
void start_research(Player *p, TechType tech);
void update_research(Player *p);

extern const TechnologyDef TECH_CATALOG[MAX_TECHNOLOGIES];

#endif // TECH_H