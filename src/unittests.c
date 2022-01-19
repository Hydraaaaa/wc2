#include <stdio.h>
#include <stdlib.h>

#include "scenario.c"
#include "pathfinder.c"

#define PATHFINDER_COUNT 2
#define BLOCKER_COUNT 7
#define DESTINATION_COUNT 2

int main()
{
	// Initialization
	Scenario* scenario = malloc(sizeof(Scenario));

	Scenario_Load(scenario, "scenarios/Gold Rush BNE.pud");

	int pathfinderX[PATHFINDER_COUNT] = {12, 13};
	int pathfinderY[PATHFINDER_COUNT] = {12, 12};

	Pathfinder* pathfinders = malloc(sizeof(Pathfinder) * PATHFINDER_COUNT);

	for (int i = 0; i < PATHFINDER_COUNT; i++)
	{
		int nodeIndex = pathfinderX[i] + pathfinderY[i] * scenario->mapSize;

		pathfinders[i].currentLocation = &scenario->pathingNodes[nodeIndex];
		pathfinders[i].currentPath = malloc(0);
		pathfinders[i].currentPathLength = 0;
		pathfinders[i].currentPathIndex = 0;
		pathfinders[i].isPathfinding = false;

		scenario->pathingNodes[nodeIndex].currentPathfinder = &pathfinders[i];
	}

	int blockerX[BLOCKER_COUNT] = {12, 13, 14, 14, 14, 13, 12};
	int blockerY[BLOCKER_COUNT] = {11, 11, 11, 12, 13, 13, 13};

	for (int i = 0; i < BLOCKER_COUNT; i++)
	{
		int nodeIndex = blockerX[i] + blockerY[i] * scenario->mapSize;

		scenario->pathingNodes[nodeIndex].type = 0b11111000;
	}

	Scenario_UpdateRegions(scenario);

	int destinationX[DESTINATION_COUNT] = {16, 16};
	int destinationY[DESTINATION_COUNT] = {12, 12};

	for (int i = 0; i < DESTINATION_COUNT; i++)
	{
		if (pathfinders[i].currentPathLength > 0)
		{
			pathfinders[i].currentPath[pathfinders[i].currentPathLength - 1]->destinedPathfinder = NULL;
		}

		pathfinders[i].currentPathLength = 0;
		pathfinders[i].currentPathIndex = 0;
		pathfinders[i].isPathfinding = true;
	}

	for (int i = 0; i < DESTINATION_COUNT; i++)
	{
		Pathfinder_GetPath(&pathfinders[i], &scenario->pathingNodes[destinationX[i] + destinationY[i] * scenario->mapSize], scenario, PATH_LAND);
	}

	int correctDestinationX[DESTINATION_COUNT] = {16, 16};
	int correctDestinationY[DESTINATION_COUNT] = {12, 11};

	bool failed = false;

	for (int i = 0; i < DESTINATION_COUNT; i++)
	{
		PathingNode* node = pathfinders[i].currentPath[pathfinders[i].currentPathLength - 1];

		if (node->posX != correctDestinationX[i] ||
			node->posY != correctDestinationY[i])
		{
			failed = true;
			printf("Pathfinder[%i] destination (%i, %i) != (%i, %i)\n", i, node->posX, node->posY, correctDestinationX[i], correctDestinationY[i]);
		}
	}

	if (failed)
	{
		printf("Pathfinding test \033[0;31mFAILED\033[0m\n");
	}
	else
	{
		printf("Pathfinding test \033[0;32mSUCCEEDED\033[0m\n");
	}

	free(pathfinders);

	free(scenario);
}
