#ifndef SCENARIO_C
#define SCENARIO_C

#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#include "pathingnode.c"

typedef struct Scenario
{
	u8 mapSize;
	u16* tiles;
	PathingNode* pathingNodes;
} Scenario;

void Scenario_Load(Scenario* scenario, char* filename)
{
	FILE* file = fopen(filename, "r");

	char header[5];

	while (fgets(header, 5, file) != NULL)
	{
		u32 length = 0;
		
		fread(&length, 4, 1, file);

		if (strcmp(header, "DIM ") == 0)
		{
			scenario->mapSize = fgetc(file);
			fseek(file, 3, SEEK_CUR);
		}
		else if (strcmp(header, "MTXM") == 0)
		{
			scenario->tiles = malloc(length);

			fread(scenario->tiles, 2, length / 2, file);
		}
		else if (strcmp(header, "SQM ") == 0)
		{
			int elementCount = length / 2;

			scenario->pathingNodes = malloc(sizeof(PathingNode) * elementCount);

			for (int i = 0; i < elementCount; i++)
			{
				scenario->pathingNodes[i].currentPathfinder = NULL;
				scenario->pathingNodes[i].destinedPathfinder = NULL;
			}

			for (int i = 0; i < elementCount; i++)
			{
				u16 input;
				fread(&input, 2, 1, file);

				// Compressing the two bytes of data into 1 byte
				// 0b00000001 = land pathable
				// 0b00000010 = water pathable
				// 0b00000100 = air pathable
				switch (input)
				{
					case 0x00: scenario->pathingNodes[i].type = 0b00000111; break; // Bridge
					case 0x01: scenario->pathingNodes[i].type = 0b00000101; break; // Land
					case 0x02: scenario->pathingNodes[i].type = 0b00000101; break; // Coast
					case 0x11: scenario->pathingNodes[i].type = 0b00000101; break; // Dirt
					case 0x40: scenario->pathingNodes[i].type = 0b00000110; break; // Water
					case 0x81: scenario->pathingNodes[i].type = 0b00000100; break; // Forest and Mountains
					case 0x82: scenario->pathingNodes[i].type = 0b00000101; break; // Coast
					case 0x8d: scenario->pathingNodes[i].type = 0b00000100; break; // Walls
					default: scenario->pathingNodes[i].type = 0b00000000; printf("[%i, %i] %i ", i % scenario->mapSize, i / scenario->mapSize, input); break;
				}

				scenario->pathingNodes[i].posX = i % scenario->mapSize;
				scenario->pathingNodes[i].posY = i / scenario->mapSize;

				u8 connectionCount = 0;

				// If not at the left edge of the map
				if (i > scenario->mapSize)
				{
					// If not at the top edge of the map
					if (i % scenario->mapSize > 0)
					{
						scenario->pathingNodes[i].connections[connectionCount] = &scenario->pathingNodes[i - scenario->mapSize - 1];
						scenario->pathingNodes[i].connectionCosts[connectionCount] = 14;
						connectionCount++;
					}

					scenario->pathingNodes[i].connections[connectionCount] = &scenario->pathingNodes[i - scenario->mapSize];
					scenario->pathingNodes[i].connectionCosts[connectionCount] = 10;
					connectionCount++;

					// If not at the bottom edge of the map
					if (i % scenario->mapSize < scenario->mapSize - 1)
					{
						scenario->pathingNodes[i].connections[connectionCount] = &scenario->pathingNodes[i - scenario->mapSize + 1];
						scenario->pathingNodes[i].connectionCosts[connectionCount] = 14;
						connectionCount++;
					}
				}

				// If not at the right edge of the map
				if (i < elementCount - scenario->mapSize)
				{
					// If not at the top edge of the map
					if (i % scenario->mapSize > 0)
					{
						scenario->pathingNodes[i].connections[connectionCount] = &scenario->pathingNodes[i + scenario->mapSize - 1];
						scenario->pathingNodes[i].connectionCosts[connectionCount] = 14;
						connectionCount++;
					}

					scenario->pathingNodes[i].connections[connectionCount] = &scenario->pathingNodes[i + scenario->mapSize];
					scenario->pathingNodes[i].connectionCosts[connectionCount] = 10;
					connectionCount++;

					// If not at the bottom edge of the map
					if (i % scenario->mapSize < scenario->mapSize - 1)
					{
						scenario->pathingNodes[i].connections[connectionCount] = &scenario->pathingNodes[i + scenario->mapSize + 1];
						scenario->pathingNodes[i].connectionCosts[connectionCount] = 14;
						connectionCount++;
					}
				}

				// If not at the top edge of the map
				if (i % scenario->mapSize > 0)
				{
					scenario->pathingNodes[i].connections[connectionCount] = &scenario->pathingNodes[i - 1];
					scenario->pathingNodes[i].connectionCosts[connectionCount] = 10;
					connectionCount++;
				}

				// If not at the bottom edge of the map
				if (i % scenario->mapSize < scenario->mapSize - 1)
				{
					scenario->pathingNodes[i].connections[connectionCount] = &scenario->pathingNodes[i + 1];
					scenario->pathingNodes[i].connectionCosts[connectionCount] = 10;
					connectionCount++;
				}

				scenario->pathingNodes[i].connectionCount = connectionCount;
			}
		}
		else
		{
			fseek(file, length, SEEK_CUR);
		}
	}

	fclose(file);

	Scenario_UpdateRegions(scenario);
}

#endif
