#ifndef SCENARIO_H
#define SCENARIO_H

#include "pathingnode.h"
#include "int.h"

typedef struct Scenario Scenario;

void Scenario_Load(Scenario* scenario, char* filename);

void Scenario_UpdateRegions(Scenario* scenario);

#endif
