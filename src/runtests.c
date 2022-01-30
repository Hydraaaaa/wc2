#include <stdio.h>
#include "test.c"
#include <time.h>

void Test_Load(Test* test, char* filePath)
{
	FILE* file = fopen(filePath, "rb");

	Test_Init(test);

	u16 actionCount;

	// Read u16 action count
	fread(&actionCount, 2, 1, file);

	TestAction* action;
	TestFrameData* frameData;

	bool firstFrameData = true;

	for (int i = 0; i < actionCount; i++)
	{
		TestAction* prevAction = action;

		action = malloc(sizeof(TestAction));

		if (i == 0)
		{
			test->firstAction = action;
		}
		else
		{
			prevAction->next = action;
		}

		// Read u8 action
		fread(&action->action, 1, 1, file);

		bool readFrameData = false;

		switch(action->action)
		{
			case ACTION_SELECT:
				// Read u8 param1, u8 param2, u8 param3, u8 param4
				fread(&action->param1, 1, 4, file);
				break;
			case ACTION_PATHFIND:
				// Read u8 param1, u8 param2
				fread(&action->param1, 1, 2, file);
				readFrameData = true;
				break;
			case ACTION_MOVE:
				readFrameData = true;
				break;
			case ACTION_SPAWN_GRUNT:
				// Read u8 param1, u8 param2
				fread(&action->param1, 1, 2, file);
				break;
			case ACTION_SPAWN_BLOCKER:
				// Read u8 param1, u8 param2
				fread(&action->param1, 1, 2, file);
				break;
		}

		if (readFrameData)
		{
			TestFrameData* prevFrameData = frameData;

			frameData = malloc(sizeof(TestFrameData));

			if (firstFrameData)
			{
				test->firstFrameData = frameData;
				firstFrameData = false;
			}
			else
			{
				prevFrameData->next = frameData;
			}

			// Read u16 pathfinder count
			fread(&frameData->pathfinderCount, 2, 1, file);

			frameData->pathfinderPositions = malloc(sizeof(Point) * frameData->pathfinderCount);
			frameData->pathfinderPaths = malloc(sizeof(Point*) * frameData->pathfinderCount);
			frameData->pathfinderPathLengths = malloc(sizeof(u16) * frameData->pathfinderCount);

			for (int i = 0; i < frameData->pathfinderCount; i++)
			{
				// Read u8 x, u8 y
				fread(&frameData->pathfinderPositions[i].x, 1, 2, file);

				// Read u16 path length
				fread(&frameData->pathfinderPathLengths[i], 2, 1, file);

				frameData->pathfinderPaths[i] = malloc(sizeof(Point) * frameData->pathfinderPathLengths[i]);

				for (int j = 0; j < frameData->pathfinderPathLengths[i]; j++)
				{
					// Read u8 x, u8 y
					fread(&frameData->pathfinderPaths[i][j].x, 1, 2, file);
				}
			}
		}
	}

	test->lastAction = action;
	test->lastFrameData = frameData;

	action->next = NULL;
	frameData->next = NULL;

	fclose(file);
}

void RunTest(char* filePath)
{
	bool success = true;

	Test test;

	Test_Load(&test, filePath);

	Scenario* scenario = malloc(sizeof(Scenario));

	Scenario_Load(scenario, "scenarios/Garden of War Classic BNE.pud");

	// Pathfinder list
	Pathfinder* pathfinders = malloc(sizeof(Pathfinder) * 128);
	int pathfinderCount = 0;

	// Debug Pathing Blockers
	PathingNode* pathingBlockers[128];
	u8 pathingBlockerCount = 0;

	Pathfinder* selectedUnits[128];
	int selectedUnitCount = 0;

	TestAction* action = test.firstAction;
	TestFrameData* frameData = test.firstFrameData;

	time_t time = 0;

	while (action != NULL)
	{
		bool checkFrameData = false;

		time_t timeSegment = clock();

		switch (action->action)
		{
			case ACTION_SPAWN_GRUNT:
			{
				if (pathfinderCount >= 128)
				{
					printf("Pathfinder limit reached, not spawning\n");
				}
				else
				{
					int x = action->param1;
					int y = action->param2;

					int nodeIndex = x + y * scenario->mapSize;

					if (scenario->pathingNodes[nodeIndex].currentPathfinder == NULL &&
							(scenario->pathingNodes[nodeIndex].type & PATH_LAND) != 0)
					{
						pathfinders[pathfinderCount].currentLocation = &scenario->pathingNodes[nodeIndex];
						pathfinders[pathfinderCount].currentPath = malloc(0);
						pathfinders[pathfinderCount].currentPathLength = 0;
						pathfinders[pathfinderCount].currentPathIndex = 0;
						pathfinders[pathfinderCount].pathingFlag = PATH_LAND;
						pathfinders[pathfinderCount].isPathfinding = false;

						scenario->pathingNodes[nodeIndex].currentPathfinder = &pathfinders[pathfinderCount];

						pathfinderCount++;

						Scenario_UpdateRegions(scenario);
					}
				}

				break;
			}
			case ACTION_SPAWN_BLOCKER:
			{
				int x = action->param1;
				int y = action->param2;

				pathingBlockers[pathingBlockerCount] = &scenario->pathingNodes[x + y * scenario->mapSize];
				pathingBlockers[pathingBlockerCount]->type = 0b11111000;

				pathingBlockerCount++;

				Scenario_UpdateRegions(scenario);

				break;
			}
			case ACTION_SELECT:
			{
				int startX = action->param1;
				int startY = action->param2;
				int endX = action->param3;
				int endY = action->param4;

				selectedUnitCount = 0;

				for (int y = startY; y < endY; y++)
				{
					int yIndex = y * scenario->mapSize;

					for (int x = startX; x < endX; x++)
					{
						if (scenario->pathingNodes[yIndex + x].currentPathfinder != NULL)
						{
							selectedUnits[selectedUnitCount] = scenario->pathingNodes[yIndex + x].currentPathfinder;
							selectedUnitCount++;
						}
					}
				}

				break;
			}
			case ACTION_PATHFIND:
			{
				checkFrameData = true;

				int x = action->param1;
				int y = action->param2;

				// Check if selected units are all contained within a 4x4 box
				int minX = selectedUnits[0]->currentLocation->posX;
				int maxX = minX;
				int minY = selectedUnits[0]->currentLocation->posY;
				int maxY = minY;

				bool formationMove = true;

				for (int i = 1; i < selectedUnitCount; i++)
				{
					PathingNode* location = selectedUnits[i]->currentLocation;

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
					int offsetX = x - ((maxX - minX) / 2 + minX);
					int offsetY = y - ((maxY - minY) / 2 + minY);

					// Maintain formation at destination
					CommandPathfindersFormation(selectedUnits, selectedUnitCount, offsetX, offsetY, scenario);
				}
				else
				{
					// Just clump around the destination
					CommandPathfindersPoint(selectedUnits, selectedUnitCount, &scenario->pathingNodes[x + y * scenario->mapSize], scenario);
				}

				break;
			}
			case ACTION_MOVE:
			{
				checkFrameData = true;

				// Reset pathfinders' moved state
				for (int i = 0; i < pathfinderCount; i++)
				{
					pathfinders[i].hasMoved = false;
				}

				Pathfinder* dependencyStack[32];

				for (int i = 0; i < pathfinderCount; i++)
				{
					if (!pathfinders[i].hasMoved)
					{
						Pathfinder_Move(&pathfinders[i], dependencyStack, 0, scenario);
					}
				}

				Scenario_UpdateRegions(scenario);

				break;
			}
		}

		time += clock() - timeSegment;

		if (checkFrameData)
		{
			if (frameData->pathfinderCount != pathfinderCount)
			{
				printf("%i pathfinders, expected %i\n", pathfinderCount, frameData->pathfinderCount);
				success = false;
				break;
			}

			for (int i = 0; i < frameData->pathfinderCount; i++)
			{
				Point* framePoint = &frameData->pathfinderPositions[i];
				PathingNode* node = pathfinders[i].currentLocation;

				if (framePoint->x != node->posX ||
						framePoint->y != node->posY)
				{
					printf("pathfinders[%i] at (%i, %i), expected (%i, %i)\n", i, node->posX, node->posY, framePoint->x, framePoint->y);
					success = false;
					break;
				}

				if (frameData->pathfinderPathLengths[i] != pathfinders[i].currentPathLength)
				{
					printf("pathfinders[%i] path length %i, expected %i\n", i, pathfinders[i].currentPathLength, frameData->pathfinderPathLengths[i]);
					success = false;
					break;
				}

				for (int j = 0; j < frameData->pathfinderPathLengths[i]; j++)
				{
					Point* framePoint = &frameData->pathfinderPaths[i][j];
					PathingNode* node = pathfinders[i].currentPath[j];

					if (framePoint->x != node->posX ||
						framePoint->y != node->posY)
					{
						printf("pathfinders[%i] path[%i] at (%i, %i), expected (%i, %i)\n", i, j, node->posX, node->posY, framePoint->x, framePoint->y);
						success = false;
						break;
					}
				}
			}

			frameData = frameData->next;
		}

		action = action->next;
	}

	time = clock() - time;

	if (success)
	{
		printf("%s \033[0;32mPASSED\033[0m in %f\n", filePath, (double)time / CLOCKS_PER_SEC);
		//printf("%s \033[0;32mPASSED\033[0m\n", filePath);
	}
	else
	{
		printf("%s \033[0;31mFAILED\033[0m\n", filePath);
	}

	free(pathfinders);

	free(scenario);

	action = test.firstAction;

	while (action != NULL)
	{
		TestAction* freeAction = action;

		action = action->next;

		free(freeAction);
	}

	frameData = test.firstFrameData;

	while (frameData != NULL)
	{
		TestFrameData* freeFrameData = frameData;

		frameData = frameData->next;

		free(freeFrameData->pathfinderPositions);
		free(freeFrameData->pathfinderPathLengths);

		for (int i = 0; i < freeFrameData->pathfinderCount; i++)
		{
			free(freeFrameData->pathfinderPaths[i]);
		}

		free(freeFrameData);
	}
}

int main()
{
	RunTest("tests/basicmove.test");
	RunTest("tests/grouparoundtrees.test");
	RunTest("tests/multiplelongformationpaths.test");
	RunTest("tests/5x5paths.test");

	return 0;
}
