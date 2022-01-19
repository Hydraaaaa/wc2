#ifndef PATHFINDER_H
#define PATHFINDER_H

#include "pathingnode.h"
#include "scenario.h"
#include "int.h"
#include <stdbool.h>

typedef enum PathingFlag
{
	PATH_LAND =  0b00000001,
	PATH_WATER = 0b00000010,
	PATH_AIR =   0b00000100
} PathingFlag;

typedef struct Pathfinder Pathfinder;

// Perform an astar search to get a sequence of nodes leading to the specified destination
void Pathfinder_GetPath(Pathfinder* pathfinder, PathingNode* destination, Scenario* scenario, PathingFlag pathingFlag);
bool Pathfinder_GetPath_Debug(Pathfinder* pathfinder, PathingNode* destination, Scenario* scenario, PathingFlag pathingFlag, PathingNode** openSet, u8* openSetLength, bool firstRun);

// Perform a dijkstra search to get the closest available node to the destination
PathingNode* Pathfinder_GetNearestDestination(Pathfinder* pathfinder, PathingNode* destination, Scenario* scenario, PathingFlag pathingFlag);

#endif
