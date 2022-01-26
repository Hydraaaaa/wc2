#include <stdio.h>
#include <stdlib.h>
#include <time.h>

#include "../scenario.c"
#include "../pathfinder.c"

typedef struct Point
{
	u16 x;
	u16 y;
} Point;

typedef struct
{
	Point startPos;
	Point* path;
	u8 pathLength;
} PathfinderResult;

void pathfinding1()
{
	// Initialization
	Scenario* scenario = malloc(sizeof(Scenario));

	Scenario_Load(scenario, "scenarios/Garden of War Classic BNE.pud");

	PathfinderResult* pathfinderResults;
	const int pathfinderCount = 4;

	Point* blockers;
	const int blockerCount = 11;

	pathfinderResults = malloc(sizeof(PathfinderResult) * pathfinderCount);
	pathfinderResults[0].startPos = (Point){8, 102};
	pathfinderResults[0].pathLength = 8;
	pathfinderResults[0].path = malloc(sizeof(Point) * 8);
	pathfinderResults[0].path[0] = (Point){7, 101};
	pathfinderResults[0].path[1] = (Point){8, 100};
	pathfinderResults[0].path[2] = (Point){9, 100};
	pathfinderResults[0].path[3] = (Point){10, 100};
	pathfinderResults[0].path[4] = (Point){11, 100};
	pathfinderResults[0].path[5] = (Point){12, 100};
	pathfinderResults[0].path[6] = (Point){13, 101};
	pathfinderResults[0].path[7] = (Point){14, 102};
	pathfinderResults[1].startPos = (Point){9, 102};
	pathfinderResults[1].pathLength = 10;
	pathfinderResults[1].path = malloc(sizeof(Point) * 10);
	pathfinderResults[1].path[0] = (Point){8, 102};
	pathfinderResults[1].path[1] = (Point){7, 101};
	pathfinderResults[1].path[2] = (Point){8, 100};
	pathfinderResults[1].path[3] = (Point){9, 100};
	pathfinderResults[1].path[4] = (Point){10, 100};
	pathfinderResults[1].path[5] = (Point){11, 100};
	pathfinderResults[1].path[6] = (Point){12, 100};
	pathfinderResults[1].path[7] = (Point){13, 101};
	pathfinderResults[1].path[8] = (Point){14, 102};
	pathfinderResults[1].path[9] = (Point){15, 102};
	pathfinderResults[2].startPos = (Point){10, 102};
	pathfinderResults[2].pathLength = 12;
	pathfinderResults[2].path = malloc(sizeof(Point) * 12);
	pathfinderResults[2].path[0] = (Point){9, 102};
	pathfinderResults[2].path[1] = (Point){8, 102};
	pathfinderResults[2].path[2] = (Point){7, 101};
	pathfinderResults[2].path[3] = (Point){8, 100};
	pathfinderResults[2].path[4] = (Point){9, 100};
	pathfinderResults[2].path[5] = (Point){10, 100};
	pathfinderResults[2].path[6] = (Point){11, 100};
	pathfinderResults[2].path[7] = (Point){12, 100};
	pathfinderResults[2].path[8] = (Point){13, 100};
	pathfinderResults[2].path[9] = (Point){14, 100};
	pathfinderResults[2].path[10] = (Point){15, 101};
	pathfinderResults[2].path[11] = (Point){16, 102};
	pathfinderResults[3].startPos = (Point){11, 102};
	pathfinderResults[3].pathLength = 14;
	pathfinderResults[3].path = malloc(sizeof(Point) * 14);
	pathfinderResults[3].path[0] = (Point){10, 102};
	pathfinderResults[3].path[1] = (Point){9, 102};
	pathfinderResults[3].path[2] = (Point){8, 102};
	pathfinderResults[3].path[3] = (Point){7, 101};
	pathfinderResults[3].path[4] = (Point){8, 100};
	pathfinderResults[3].path[5] = (Point){9, 100};
	pathfinderResults[3].path[6] = (Point){10, 100};
	pathfinderResults[3].path[7] = (Point){11, 100};
	pathfinderResults[3].path[8] = (Point){12, 100};
	pathfinderResults[3].path[9] = (Point){13, 100};
	pathfinderResults[3].path[10] = (Point){14, 100};
	pathfinderResults[3].path[11] = (Point){15, 101};
	pathfinderResults[3].path[12] = (Point){16, 102};
	pathfinderResults[3].path[13] = (Point){17, 102};
	blockers = malloc(sizeof(Point) * blockerCount);
	blockers[0] = (Point){8, 101};
	blockers[1] = (Point){9, 101};
	blockers[2] = (Point){10, 101};
	blockers[3] = (Point){11, 101};
	blockers[4] = (Point){12, 101};
	blockers[5] = (Point){12, 102};
	blockers[6] = (Point){12, 103};
	blockers[7] = (Point){11, 103};
	blockers[8] = (Point){10, 103};
	blockers[9] = (Point){9, 103};
	blockers[10] = (Point){8, 103};

	Pathfinder* pathfinders = malloc(sizeof(Pathfinder) * pathfinderCount);

	for (int i = 0; i < pathfinderCount; i++)
	{
		int nodeIndex = pathfinderResults[i].startPos.x + pathfinderResults[i].startPos.y * scenario->mapSize;

		pathfinders[i].currentLocation = &scenario->pathingNodes[nodeIndex];
		pathfinders[i].currentPath = malloc(0);
		pathfinders[i].currentPathLength = 0;
		pathfinders[i].currentPathIndex = 0;
		pathfinders[i].pathingFlag = PATH_LAND;
		pathfinders[i].isPathfinding = false;

		scenario->pathingNodes[nodeIndex].currentPathfinder = &pathfinders[i];
	}

	for (int i = 0; i < blockerCount; i++)
	{
		int nodeIndex = blockers[i].x + blockers[i].y * scenario->mapSize;

		scenario->pathingNodes[nodeIndex].type = 0b11111000;
	}

	Scenario_UpdateRegions(scenario);

	clock_t time = clock();

	for (int i = 0; i < pathfinderCount; i++)
	{
		if (pathfinders[i].currentPathLength > 0)
		{
			pathfinders[i].currentPath[pathfinders[i].currentPathLength - 1]->destinedPathfinder = NULL;
		}

		pathfinders[i].currentPathLength = 0;
		pathfinders[i].currentPathIndex = 0;
		pathfinders[i].isPathfinding = true;
	}

	Pathfinder** selectedPathfinders = malloc(sizeof(Pathfinder*) * pathfinderCount);

	for (int i = 0; i < pathfinderCount; i++)
	{
		selectedPathfinders[i] = &pathfinders[i];
	}

	int mouseX = 15;
	int mouseY = 102;

	// Check if selected units are all contained within a 4x4 box
	int minX = pathfinders[0].currentLocation->posX;
	int maxX = minX;
	int minY = pathfinders[0].currentLocation->posY;
	int maxY = minY;

	bool formationMove = true;

	for (int i = 1; i < pathfinderCount; i++)
	{
		PathingNode* location = selectedPathfinders[i]->currentLocation;

		if (location->posX > maxX)
		{
			maxX = location->posX;
		}

		if (location->posX < minX)
		{
			minX = location->posX;
		}

		if (location->posY > maxY)
		{
			maxY = location->posY;
		}

		if (location->posY < minY)
		{
			minY = location->posY;
		}

		if (maxX - minX > 3 ||
		    maxY - minY > 3)
		{
			formationMove = false;
			break;
		}
	}

	// If units are contained within a 4x4 box
	if (formationMove)
	{
		int offsetX = mouseX - ((maxX - minX) / 2 + minX);
		int offsetY = mouseY - ((maxY - minY) / 2 + minY);

		// Maintain formation at destination
		CommandPathfindersFormation(selectedPathfinders, pathfinderCount, offsetX, offsetY, scenario);
	}
	else
	{
		// Just clump around the destination
		CommandPathfindersPoint(selectedPathfinders, pathfinderCount, &scenario->pathingNodes[mouseX + mouseY * scenario->mapSize], scenario);
	}

	time = clock() - time;

	bool failed = false;

	for (int i = 0; i < pathfinderCount; i++)
	{
		if (pathfinders[i].currentLocation->posX != pathfinderResults[i].startPos.x ||
			pathfinders[i].currentLocation->posY != pathfinderResults[i].startPos.y)
		{
			failed = true;
			printf
			(
				"Pathfinder[%i] startPos (%i, %i) != (%i, %i)\n",
				i,
				pathfinders[i].currentLocation->posX,
				pathfinders[i].currentLocation->posY,
				pathfinderResults[i].startPos.x,
				pathfinderResults[i].startPos.y
			);

			break;
		}

		for (int j = 0; j < pathfinderResults[i].pathLength; j++)
		{
			PathingNode* node = pathfinders[i].currentPath[j];

			if (node->posX != pathfinderResults[i].path[j].x ||
				node->posY != pathfinderResults[i].path[j].y)
			{
				failed = true;
				printf
				(
					"Pathfinder[%i] pathNode[%i] (%i, %i) != (%i, %i)\n",
					i,
					j,
					node->posX,
					node->posY,
					pathfinderResults[i].path[j].x,
					pathfinderResults[i].path[j].y
				);

				break;
			}
		}
	}

	if (!failed)
	{
		printf("Pathfinding test \033[0;32mSUCCEEDED\033[0m in %f\n", (double)time / CLOCKS_PER_SEC);
	}
	else
	{
		printf("Pathfinding test \033[0;31mFAILED\033[0m\n");
	}


	free(pathfinders);
	free(blockers);

	for (int i = 0; i < pathfinderCount; i++)
	{
		free(pathfinderResults[i].path);
	}

	free(pathfinderResults);
	free(scenario);
}
