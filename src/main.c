#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include "raylib.h"

#include "scenario.c"
#include "pathfinder.c"
#include "test.c"

#define SCREEN_WIDTH 1280
#define SCREEN_HEIGHT 720

#define TERRAIN_SPRITESHEET_WIDTH 18
#define TERRAIN_SPRITESHEET_HEIGHT 20

int main()
{
    InitWindow(SCREEN_WIDTH, SCREEN_HEIGHT, "Warcraft II");

    SetTargetFPS(100);

	char fpsString[8];

	Texture2D terrainSpriteSheet = LoadTexture("summer-tileset.png");
	Texture2D gruntSpriteSheet = LoadTexture("grunt.png");

	float cameraPosX = 0, cameraPosY = 0;

	Scenario* scenario = malloc(sizeof(Scenario));

	Scenario_Load(scenario, "scenarios/Garden of War BNE.pud");

	// Pathfinder list
	Pathfinder* pathfinders = malloc(sizeof(Pathfinder) * 128);
	int pathfinderCount = 0;

	// Debug Pathing Blockers
	PathingNode* pathingBlockers[128];
	u8 pathingBlockerCount = 0;

	// Mouse selection
	float mouseStartPosX, mouseStartPosY;
	bool dragging = false;

	Pathfinder* selectedUnits[128];
	int selectedUnitCount = 0;

	// Debug smooth frames square
	int squarePhase = 0;
	float squareProgress = 0.0f;

	// Test
	Test test;

	Test_Init(&test);

    // Main game loop
    while (!WindowShouldClose())
    {
		float time = GetFrameTime();

		// Camera controls
		float cameraSpeed = time * 1024.0f;

		if (IsKeyDown(KEY_UP))
		{
			cameraPosY += cameraSpeed;
		}

		if (IsKeyDown(KEY_DOWN))
		{
			cameraPosY -= cameraSpeed;
		}

		if (IsKeyDown(KEY_LEFT))
		{
			cameraPosX += cameraSpeed;
		}

		if (IsKeyDown(KEY_RIGHT))
		{
			cameraPosX -= cameraSpeed;
		}

		// Unit Selection
		if (IsMouseButtonPressed(MOUSE_LEFT_BUTTON))
		{
			mouseStartPosX = GetMouseX() - cameraPosX;
			mouseStartPosY = GetMouseY() - cameraPosY;

			dragging = true;
		}

		if (IsMouseButtonReleased(MOUSE_LEFT_BUTTON))
		{
			float startX = (mouseStartPosX + 16) / 32 - 0.5f;
			float startY = (mouseStartPosY + 16) / 32 - 0.5f;

			float endX = (GetMouseX() - cameraPosX + 16) / 32 - 0.5f;
			float endY = (GetMouseY() - cameraPosY + 16) / 32 - 0.5f;

			if (startX < 0)
			{
				startX = 0;
			}
			else if (startX >= scenario->mapSize)
			{
				startX = scenario->mapSize - 1;
			}

			if (startY < 0)
			{
				startY = 0;
			}
			else if (startY >= scenario->mapSize)
			{
				startY = scenario->mapSize - 1;
			}

			if (endX < 0)
			{
				endX = 0;
			}
			else if (endX >= scenario->mapSize)
			{
				endX = scenario->mapSize - 1;
			}

			if (endY < 0)
			{
				endY = 0;
			}
			else if (endY >= scenario->mapSize)
			{
				endY = scenario->mapSize - 1;
			}

			float lowestX = startX;
			float highestX = endX;

			float lowestY = startY;
			float highestY = endY;

			if (endX < startX)
			{
				lowestX = endX;
				highestX = startX;
			}

			if (endY < startY)
			{
				lowestY = endY;
				highestY = startY;
			}

			lowestX = floor(lowestX);
			lowestY = floor(lowestY);
			highestX = ceil(highestX);
			highestY = ceil(highestY);

			selectedUnitCount = 0;

			for (int y = lowestY; y < highestY; y++)
			{
				int yIndex = y * scenario->mapSize;

				for (int x = lowestX; x < highestX; x++)
				{
					if (scenario->pathingNodes[yIndex + x].currentPathfinder != NULL)
					{
						selectedUnits[selectedUnitCount] = scenario->pathingNodes[yIndex + x].currentPathfinder;
						selectedUnitCount++;
					}
				}
			}

			dragging = false;

			Test_AllocateAction(&test);

			test.lastAction->action = ACTION_SELECT;
			test.lastAction->param1 = lowestX;
			test.lastAction->param2 = lowestY;
			test.lastAction->param3 = highestX;
			test.lastAction->param4 = highestY;
		}

		if (IsMouseButtonPressed(MOUSE_RIGHT_BUTTON))
		{
			if (selectedUnitCount > 0)
			{
				int mouseX = (GetMouseX() - cameraPosX) / 32;
				int mouseY = (GetMouseY() - cameraPosY) / 32;

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
					int offsetX = mouseX - ((maxX - minX) / 2 + minX);
					int offsetY = mouseY - ((maxY - minY) / 2 + minY);

					// Maintain formation at destination
					CommandPathfindersFormation(selectedUnits, selectedUnitCount, offsetX, offsetY, scenario);
				}
				else
				{
					// Just clump around the destination
					CommandPathfindersPoint(selectedUnits, selectedUnitCount, &scenario->pathingNodes[mouseX + mouseY * scenario->mapSize], scenario);
				}

				Test_AllocateAction(&test);

				test.lastAction->action = ACTION_PATHFIND;
				test.lastAction->param1 = mouseX;
				test.lastAction->param2 = mouseY;

				Test_AllocateFrameData(&test);
				TestFrameData_Populate(test.lastFrameData, pathfinders, pathfinderCount);
			}
		}

		// Grunt Spawner
		if (IsKeyPressed(KEY_A))
		{
			if (pathfinderCount >= 128)
			{
				printf("Pathfinder limit reached, not spawning\n");
			}
			else
			{
				int mouseX = (GetMouseX() - cameraPosX) / 32;
				int mouseY = (GetMouseY() - cameraPosY) / 32;

				int nodeIndex = mouseX + mouseY * scenario->mapSize;

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

					Test_AllocateAction(&test);

					test.lastAction->action = ACTION_SPAWN_GRUNT;
					test.lastAction->param1 = mouseX;
					test.lastAction->param2 = mouseY;
				}
			}
		}

		// Pathing Blocker Spawner
		if (IsKeyPressed(KEY_B))
		{
			int mouseX = (GetMouseX() - cameraPosX) / 32;
			int mouseY = (GetMouseY() - cameraPosY) / 32;

			pathingBlockers[pathingBlockerCount] = &scenario->pathingNodes[mouseX + mouseY * scenario->mapSize];
			pathingBlockers[pathingBlockerCount]->type = 0b11111000;

			pathingBlockerCount++;

			Scenario_UpdateRegions(scenario);

			Test_AllocateAction(&test);

			test.lastAction->action = ACTION_SPAWN_BLOCKER;
			test.lastAction->param1 = mouseX;
			test.lastAction->param2 = mouseY;
		}

		if (IsKeyPressed(KEY_M))
		{
			// Reset pathfinders' moved state
			for (int i = 0; i < pathfinderCount; i++)
			{
				pathfinders[i].hasMoved = false;
			}

			Pathfinder* dependencyStack[128];

			for (int i = 0; i < pathfinderCount; i++)
			{
				if (!pathfinders[i].hasMoved)
				{
					Pathfinder_Move(&pathfinders[i], dependencyStack, 0, scenario);
				}
			}

			Test_AllocateAction(&test);

			test.lastAction->action = ACTION_MOVE;

			Test_AllocateFrameData(&test);
			TestFrameData_Populate(test.lastFrameData, pathfinders, pathfinderCount);
		}

		// Save test
		if (IsKeyPressed(KEY_T))
		{
			FILE* file = fopen("newtest.test", "wb");

			TestAction* currentAction = test.firstAction;
			TestFrameData* currentFrameData = test.firstFrameData;

			u16 actionCount = 0;

			while (currentAction != NULL)
			{
				actionCount++;
				currentAction = currentAction->next;
			}

			// Write u16 action count
			fwrite(&actionCount, 2, 1, file);

			currentAction = test.firstAction;

			while (currentAction != NULL)
			{
				// Write u8 action
				fwrite(&currentAction->action, 1, 1, file);

				bool writeFrameData = false;

				switch (currentAction->action)
				{
					case ACTION_SELECT:
						// Write u8 param1, u8 param2, u8 param3, u8 param4
						fwrite(&currentAction->param1, 1, 4, file);
						break;
					case ACTION_PATHFIND:
						// Write u8 param1, u8 param2
						fwrite(&currentAction->param1, 1, 2, file);
						writeFrameData = true;
						break;
					case ACTION_MOVE:
						writeFrameData = true;
						break;
					case ACTION_SPAWN_GRUNT:
						// Write u8 param1, u8 param2
						fwrite(&currentAction->param1, 1, 2, file);
						break;
					case ACTION_SPAWN_BLOCKER:
						// Write u8 param1, u8 param2
						fwrite(&currentAction->param1, 1, 2, file);
						break;
				}

				currentAction = currentAction->next;

				if (writeFrameData)
				{
					// Write u16 pathfinder count
					fwrite(&currentFrameData->pathfinderCount, 2, 1, file);

					for (int i = 0; i < currentFrameData->pathfinderCount; i++)
					{
						// Write u8 x, u8 y
						fwrite(&currentFrameData->pathfinderPositions[i].x, 1, 2, file);

						// Write u16 path length
						fwrite(&currentFrameData->pathfinderPathLengths[i], 2, 1, file);

						for (int j = 0; j < currentFrameData->pathfinderPathLengths[i]; j++)
						{
							// Write u8 x, u8 y
							fwrite(&currentFrameData->pathfinderPaths[i][j].x, 1, 2, file);
						}
					}

					currentFrameData = currentFrameData->next;
				}
			}

			fclose(file);
		}


		// Rendering <><><><><><><><><><><><><><><><><><><><><><><><><><><><>


        BeginDrawing();

		ClearBackground(BLACK);

		// Draw Terrain
		for (int y = 0; y < scenario->mapSize; y++)
		{
			int yIndex = y * scenario->mapSize;

			for (int x = 0; x < scenario->mapSize; x++)
			{
				bool draw = true;
				Rectangle rect = {32.0f, 32.0f, 32.0f, 32.0f};

				switch (scenario->tiles[yIndex + x])
				{
					case 0x030: rect.x = 32.0f * 11; rect.y = 32.0f * 17; break;
					case 0x031: rect.x = 32.0f * 12; rect.y = 32.0f * 17; break;
					case 0x032: rect.x = 32.0f * 13; rect.y = 32.0f * 17; break;
					case 0x034: rect.x = 32.0f * 14; rect.y = 32.0f * 17; break;
					case 0x035: rect.x = 32.0f * 15; rect.y = 32.0f * 17; break;
					case 0x036: rect.x = 32.0f * 16; rect.y = 32.0f * 17; break;
					case 0x037: rect.x = 32.0f * 17; rect.y = 32.0f * 17; break;
					case 0x038: rect.x = 32.0f * 0; rect.y = 32.0f * 18; break;
					case 0x039: rect.x = 32.0f * 1; rect.y = 32.0f * 18; break;
					case 0x03A: rect.x = 32.0f * 2; rect.y = 32.0f * 18; break;
					case 0x03B: rect.x = 32.0f * 3; rect.y = 32.0f * 18; break;
					case 0x050: rect.x = 32.0f * 15; rect.y = 32.0f * 18; break;
					case 0x051: rect.x = 32.0f * 16; rect.y = 32.0f * 18; break;
					case 0x052: rect.x = 32.0f * 15; rect.y = 32.0f * 18; break;
					case 0x054: rect.x = 32.0f * 17; rect.y = 32.0f * 18; break;
					case 0x055: rect.x = 32.0f * 0; rect.y = 32.0f * 19; break;
					case 0x056: rect.x = 32.0f * 1; rect.y = 32.0f * 19; break;
					case 0x057: rect.x = 32.0f * 2; rect.y = 32.0f * 19; break;
					case 0x058: rect.x = 32.0f * 3; rect.y = 32.0f * 19; break;
					case 0x059: rect.x = 32.0f * 4; rect.y = 32.0f * 19; break;
					case 0x05A: rect.x = 32.0f * 17; rect.y = 32.0f * 18; break;
					case 0x05B: rect.x = 32.0f * 0; rect.y = 32.0f * 19; break;
					case 0x05C: rect.x = 32.0f * 17; rect.y = 32.0f * 18; break;
					case 0x05D: rect.x = 32.0f * 0; rect.y = 32.0f * 19; break;
					case 0x05E: rect.x = 32.0f * 17; rect.y = 32.0f * 18; break;
					case 0x05F: rect.x = 32.0f * 0; rect.y = 32.0f * 19; break;
					case 0x060: rect.x = 32.0f * 5; rect.y = 32.0f * 19; break;
					case 0x061: rect.x = 32.0f * 6; rect.y = 32.0f * 19; break;
					case 0x062: rect.x = 32.0f * 5; rect.y = 32.0f * 19; break;
					case 0x064: rect.x = 32.0f * 7; rect.y = 32.0f * 19; break;
					case 0x065: rect.x = 32.0f * 8; rect.y = 32.0f * 19; break;
					case 0x066: rect.x = 32.0f * 9; rect.y = 32.0f * 19; break;
					case 0x067: rect.x = 32.0f * 10; rect.y = 32.0f * 19; break;
					case 0x068: rect.x = 32.0f * 11; rect.y = 32.0f * 19; break;
					case 0x069: rect.x = 32.0f * 12; rect.y = 32.0f * 19; break;
					case 0x06A: rect.x = 32.0f * 7; rect.y = 32.0f * 19; break;
					case 0x06B: rect.x = 32.0f * 8; rect.y = 32.0f * 19; break;
					case 0x06C: rect.x = 32.0f * 7; rect.y = 32.0f * 19; break;
					case 0x06D: rect.x = 32.0f * 8; rect.y = 32.0f * 19; break;
					case 0x06E: rect.x = 32.0f * 7; rect.y = 32.0f * 19; break;
					case 0x06F: rect.x = 32.0f * 8; rect.y = 32.0f * 19; break;
					case 0x070: rect.x = 32.0f * 1; rect.y = 32.0f * 6; break;
					case 0x071: rect.x = 32.0f * 3; rect.y = 32.0f * 6; break;
					case 0x072: rect.x = 32.0f * 4; rect.y = 32.0f * 6; break;
					case 0x080: rect.x = 32.0f * 5; rect.y = 32.0f * 8; break;
					case 0x081: rect.x = 32.0f * 17; rect.y = 32.0f * 8; break;
					case 0x082: rect.x = 32.0f * 0; rect.y = 32.0f * 9; break;
					case 0x083: rect.x = 32.0f * 1; rect.y = 32.0f * 9; break;
					case 0x500: rect.x = 32.0f * 1; rect.y = 32.0f * 14; break;
					case 0x501: rect.x = 32.0f * 2; rect.y = 32.0f * 14; break;
					case 0x510: rect.x = 32.0f * 3; rect.y = 32.0f * 14; break;
					case 0x511: rect.x = 32.0f * 4; rect.y = 32.0f * 14; break;
					case 0x520: rect.x = 32.0f * 5; rect.y = 32.0f * 14; break;
					case 0x521: rect.x = 32.0f * 6; rect.y = 32.0f * 14; break;
					case 0x522: rect.x = 32.0f * 7; rect.y = 32.0f * 14; break;
					case 0x530: rect.x = 32.0f * 8; rect.y = 32.0f * 14; break;
					case 0x531: rect.x = 32.0f * 9; rect.y = 32.0f * 14; break;
					case 0x540: rect.x = 32.0f * 10; rect.y = 32.0f * 14; break;
					case 0x541: rect.x = 32.0f * 11; rect.y = 32.0f * 14; break;
					case 0x542: rect.x = 32.0f * 12; rect.y = 32.0f * 14; break;
					case 0x550: rect.x = 32.0f * 13; rect.y = 32.0f * 14; break;
					case 0x551: rect.x = 32.0f * 14; rect.y = 32.0f * 14; break;
					case 0x560: rect.x = 32.0f * 15; rect.y = 32.0f * 14; break;
					case 0x561: rect.x = 32.0f * 15; rect.y = 32.0f * 14; break;
					case 0x570: rect.x = 32.0f * 16; rect.y = 32.0f * 14; break;
					case 0x571: rect.x = 32.0f * 17; rect.y = 32.0f * 14; break;
					case 0x580: rect.x = 32.0f * 0; rect.y = 32.0f * 15; break;
					case 0x581: rect.x = 32.0f * 1; rect.y = 32.0f * 15; break;
					case 0x590: rect.x = 32.0f * 2; rect.y = 32.0f * 15; break;
					case 0x591: rect.x = 32.0f * 3; rect.y = 32.0f * 15; break;
					case 0x592: rect.x = 32.0f * 4; rect.y = 32.0f * 15; break;
					case 0x5A0: rect.x = 32.0f * 5; rect.y = 32.0f * 15; break;
					case 0x5A1: rect.x = 32.0f * 5; rect.y = 32.0f * 15; break;
					case 0x5B0: rect.x = 32.0f * 6; rect.y = 32.0f * 15; break;
					case 0x5B1: rect.x = 32.0f * 7; rect.y = 32.0f * 15; break;
					case 0x5B2: rect.x = 32.0f * 8; rect.y = 32.0f * 15; break;
					case 0x5C0: rect.x = 32.0f * 9; rect.y = 32.0f * 15; break;
					case 0x5C1: rect.x = 32.0f * 10; rect.y = 32.0f * 15; break;
					case 0x5D0: rect.x = 32.0f * 11; rect.y = 32.0f * 15; break;
					case 0x5D1: rect.x = 32.0f * 12; rect.y = 32.0f * 15; break;
					case 0x600: rect.x = 32.0f * 5; rect.y = 32.0f * 12; break;
					case 0x601: rect.x = 32.0f * 6; rect.y = 32.0f * 12; break;
					case 0x610: rect.x = 32.0f * 7; rect.y = 32.0f * 12; break;
					case 0x611: rect.x = 32.0f * 8; rect.y = 32.0f * 12; break;
					case 0x620: rect.x = 32.0f * 9; rect.y = 32.0f * 12; break;
					case 0x621: rect.x = 32.0f * 10; rect.y = 32.0f * 12; break;
					case 0x622: rect.x = 32.0f * 11; rect.y = 32.0f * 12; break;
					case 0x630: rect.x = 32.0f * 12; rect.y = 32.0f * 12; break;
					case 0x631: rect.x = 32.0f * 13; rect.y = 32.0f * 12; break;
					case 0x640: rect.x = 32.0f * 14; rect.y = 32.0f * 12; break;
					case 0x641: rect.x = 32.0f * 15; rect.y = 32.0f * 12; break;
					case 0x642: rect.x = 32.0f * 16; rect.y = 32.0f * 12; break;
					case 0x650: rect.x = 32.0f * 17; rect.y = 32.0f * 12; break;
					case 0x651: rect.x = 32.0f * 0; rect.y = 32.0f * 13; break;
					case 0x660: rect.x = 32.0f * 1; rect.y = 32.0f * 13; break;
					case 0x661: rect.x = 32.0f * 2; rect.y = 32.0f * 13; break;
					case 0x670: rect.x = 32.0f * 3; rect.y = 32.0f * 13; break;
					case 0x671: rect.x = 32.0f * 4; rect.y = 32.0f * 13; break;
					case 0x680: rect.x = 32.0f * 5; rect.y = 32.0f * 13; break;
					case 0x681: rect.x = 32.0f * 6; rect.y = 32.0f * 13; break;
					case 0x690: rect.x = 32.0f * 7; rect.y = 32.0f * 13; break;
					case 0x691: rect.x = 32.0f * 8; rect.y = 32.0f * 13; break;
					case 0x692: rect.x = 32.0f * 9; rect.y = 32.0f * 13; break;
					case 0x6A0: rect.x = 32.0f * 10; rect.y = 32.0f * 13; break;
					case 0x6A1: rect.x = 32.0f * 11; rect.y = 32.0f * 13; break;
					case 0x6B0: rect.x = 32.0f * 12; rect.y = 32.0f * 13; break;
					case 0x6B1: rect.x = 32.0f * 13; rect.y = 32.0f * 13; break;
					case 0x6B2: rect.x = 32.0f * 14; rect.y = 32.0f * 13; break;
					case 0x6C0: rect.x = 32.0f * 15; rect.y = 32.0f * 13; break;
					case 0x6C1: rect.x = 32.0f * 16; rect.y = 32.0f * 13; break;
					case 0x6D0: rect.x = 32.0f * 17; rect.y = 32.0f * 13; break;
					case 0x6D1: rect.x = 32.0f * 0; rect.y = 32.0f * 14; break;
					case 0x700: rect.x = 32.0f * 4; rect.y = 32.0f * 5; break;
					case 0x701: rect.x = 32.0f * 5; rect.y = 32.0f * 6; break;
					case 0x710: rect.x = 32.0f * 14; rect.y = 32.0f * 4; break;
					case 0x711: rect.x = 32.0f * 6; rect.y = 32.0f * 6; break;
					case 0x720: rect.x = 32.0f * 0; rect.y = 32.0f * 6; break;
					case 0x721: rect.x = 32.0f * 7; rect.y = 32.0f * 6; break;
					case 0x730: rect.x = 32.0f * 1; rect.y = 32.0f * 5; break;
					case 0x731: rect.x = 32.0f * 8; rect.y = 32.0f * 6; break;
					case 0x740: rect.x = 32.0f * 9; rect.y = 32.0f * 6; break;
					case 0x741: rect.x = 32.0f * 3; rect.y = 32.0f * 5; break;
					case 0x750: rect.x = 32.0f * 15; rect.y = 32.0f * 6; break;
					case 0x751: rect.x = 32.0f * 8; rect.y = 32.0f * 5; break;
					case 0x760: rect.x = 32.0f * 5; rect.y = 32.0f * 5; break;
					case 0x761: rect.x = 32.0f * 5; rect.y = 32.0f * 5; break;
					case 0x770: rect.x = 32.0f * 16; rect.y = 32.0f * 4; break;
					case 0x771: rect.x = 32.0f * 12; rect.y = 32.0f * 6; break;
					case 0x780: rect.x = 32.0f * 16; rect.y = 32.0f * 6; break;
					case 0x781: rect.x = 32.0f * 17; rect.y = 32.0f * 6; break;
					case 0x790: rect.x = 32.0f * 15; rect.y = 32.0f * 4; break;
					case 0x791: rect.x = 32.0f * 11; rect.y = 32.0f * 6; break;
					case 0x7A0: rect.x = 32.0f * 6; rect.y = 32.0f * 5; break;
					case 0x7A1: rect.x = 32.0f * 6; rect.y = 32.0f * 5; break;
					case 0x7B0: rect.x = 32.0f * 0; rect.y = 32.0f * 5; break;
					case 0x7B1: rect.x = 32.0f * 10; rect.y = 32.0f * 6; break;
					case 0x7C0: rect.x = 32.0f * 13; rect.y = 32.0f * 6; break;
					case 0x7C1: rect.x = 32.0f * 2; rect.y = 32.0f * 5; break;
					case 0x7D0: rect.x = 32.0f * 17; rect.y = 32.0f * 4; break;
					case 0x7D1: rect.x = 32.0f * 17; rect.y = 32.0f * 4; break;
					default:	draw = false;
				}

				if (draw)
				{
					Vector2 vec = {x * 32.0f + cameraPosX, y * 32.0f + cameraPosY};

					DrawTextureRec(terrainSpriteSheet, rect, vec, WHITE);

					// Debug node co-ordinates
					//sprintf(fpsString, "%i.%i", scenario->pathingNodes[yIndex + x].posX, scenario->pathingNodes[yIndex + x].posY);
					//DrawText(fpsString, x * 32.0f + cameraPosX, y * 32.0f + cameraPosY, 10, RAYWHITE);
				}
				else
				{
					sprintf(fpsString, "%03X", scenario->tiles[yIndex + x]);

					DrawText(fpsString, x * 32.0f + cameraPosX, y * 32.0f + cameraPosY, 14, RAYWHITE);
				}
			}
		}

		// Draw Blockers
		for (int i = 0; i < pathingBlockerCount; i++)
		{
			Vector2 vec = {pathingBlockers[i]->posX * 32.0f + cameraPosX, pathingBlockers[i]->posY * 32.0f + cameraPosY};
			DrawRectangle(vec.x, vec.y, 32, 32, MAROON);
		}

		// Draw Selection Outlines
		for (int i = 0; i < selectedUnitCount; i++)
		{
			DrawRectangleLines(selectedUnits[i]->currentLocation->posX * 32.0f + cameraPosX, selectedUnits[i]->currentLocation->posY * 32.0f + cameraPosY, 32, 32, GREEN);
		}

		Color debugColors[] = { WHITE, PINK, PURPLE, YELLOW, LIGHTGRAY, ORANGE, RED, MAGENTA, VIOLET };

		int debugIndex = 0;

		// Draw Units
		for (int i = 0; i < pathfinderCount; i++)
		{
			Rectangle rect = {0.0f, 0.0f, 70.0f, 70.0f};
			Vector2 vec = {pathfinders[i].currentLocation->posX * 32.0f - 20.0f + cameraPosX, pathfinders[i].currentLocation->posY * 32.0f - 20.0f + cameraPosY};
			DrawTextureRec(gruntSpriteSheet, rect, vec, WHITE);

			// Draw debug paths
			if (pathfinders[i].currentPathIndex < pathfinders[i].currentPathLength)
			{
				DrawCircle
				(
					pathfinders[i].currentLocation->posX * 32 + cameraPosX + 16,
					pathfinders[i].currentLocation->posY * 32 + cameraPosY + 16 - debugIndex,
					4,
					debugColors[debugIndex]
				);

				for (int j = pathfinders[i].currentPathIndex; j < pathfinders[i].currentPathLength; j++)
				{
					DrawCircle
					(
						pathfinders[i].currentPath[j]->posX * 32 + cameraPosX + 16,
						pathfinders[i].currentPath[j]->posY * 32 + cameraPosY + 16 - debugIndex,
						4,
						debugColors[debugIndex]
					);
				}

				debugIndex++;

				if (debugIndex == 9)
				{
					debugIndex = 0;
				}
			}
		}

		debugIndex = 0;

		// Draw Debug Paths
		for (int i = 0; i < pathfinderCount; i++)
		{
			// Draw debug paths
			if (pathfinders[i].currentPathIndex < pathfinders[i].currentPathLength)
			{
				DrawLine
				(
					pathfinders[i].currentLocation->posX * 32 + cameraPosX + 16,
					pathfinders[i].currentLocation->posY * 32 + cameraPosY + 16 - debugIndex,
					pathfinders[i].currentPath[pathfinders[i].currentPathIndex]->posX * 32 + cameraPosX + 16,
					pathfinders[i].currentPath[pathfinders[i].currentPathIndex]->posY * 32 + cameraPosY + 16 - debugIndex,
					debugColors[debugIndex]
				);

				for (int j = pathfinders[i].currentPathIndex + 1; j < pathfinders[i].currentPathLength; j++)
				{
					DrawLine
					(
						pathfinders[i].currentPath[j - 1]->posX * 32 + cameraPosX + 16,
						pathfinders[i].currentPath[j - 1]->posY * 32 + cameraPosY + 16 - debugIndex,
						pathfinders[i].currentPath[j]->posX * 32 + cameraPosX + 16,
						pathfinders[i].currentPath[j]->posY * 32 + cameraPosY + 16 - debugIndex,
						debugColors[debugIndex]
					);
				}

				debugIndex++;

				if (debugIndex == 9)
				{
					debugIndex = 0;
				}
			}
		}

		// Draw Box Select
		if (dragging)
		{
			float startX = mouseStartPosX + cameraPosX;
			float startY = mouseStartPosY + cameraPosY;

			float endX = GetMouseX();
			float endY = GetMouseY();

			int lowestX = startX;
			int highestX = endX;

			int lowestY = startY;
			int highestY = endY;

			if (endX < startX)
			{
				lowestX = endX;
				highestX = startX;
			}

			if (endY < startY)
			{
				lowestY = endY;
				highestY = startY;
			}

			DrawRectangleLines(lowestX, lowestY, highestX - lowestX, highestY - lowestY, GREEN);
		}

		// Draw Smooth Square
		squareProgress += time;

		if (squareProgress >= 0.5f)
		{
			squareProgress -= 0.5f;
			squarePhase ++;

			if (squarePhase > 3)
			{
				squarePhase = 0;
			}
		}

		int posX, posY;

		switch (squarePhase)
		{
			case 0: posX = squareProgress * 80; posY = 40; break;
			case 1: posX = 40; posY = (0.5f - squareProgress) * 80; break;
			case 2: posX = (0.5f - squareProgress) * 80; posY = 0; break;
			case 3: posX = 0; posY = squareProgress * 80; break;
		}

		DrawRectangle(SCREEN_WIDTH - 100 + posX, SCREEN_HEIGHT - 100 + posY, 40, 40, RAYWHITE);

		sprintf(fpsString, "%i", GetFPS());

		DrawText(fpsString, 0, 0, 20, RAYWHITE);

        EndDrawing();
    }

	free(pathfinders);

	free(scenario);

	UnloadTexture(terrainSpriteSheet);
	UnloadTexture(gruntSpriteSheet);
	
    CloseWindow();
}
