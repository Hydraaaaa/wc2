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
bool Pathfinder_PathToDestination(Pathfinder* pathfinder, PathingNode* destination, Scenario* scenario);

// Perform a dijkstra search to get the closest available node to the destination
void Pathfinder_GetPath(Pathfinder* pathfinder, PathingNode* destination, Scenario* scenario);

// Basically calls Pathfinder_GetNearestDestination for pathfinders[]
void CommandPathfinders(Pathfinder* pathfinders[], int pathfinderCount, PathingNode* destination, Scenario* scenario);

void Pathfinder_Move(Pathfinder* pathfinder, Pathfinder* dependencyStack[], int dependencyStackLength, Scenario* scenario);

#endif
