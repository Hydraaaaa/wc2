#ifndef PATHFINDER_C
#define PATHFINDER_C

#include <stdio.h>
#include <stdlib.h>
#include "pathfinder.h"
#include "pathingnode.c"
#include "scenario.c"
#include <stdbool.h>

struct Pathfinder
{
	PathingNode* currentLocation;
	PathingNode* desiredLocation; // Will path as close as it can to this point, but not necessarily directly to it
	PathingNode** currentPath;
	u16 currentPathLength;
	u16 currentPathIndex;

	// 00000001 Land Pathable
	// 00000010 Water Pathable
	// 00000100 Air Pathable
	u8 pathingFlag;

	bool isPathfinding;
	bool hasMoved; // When move commands are issued, this will let other pathfinders know that this pathfinder has already moved this frame
};

// Need to store the g and h outside of the pathing node due to the conflicting use of them by GetPath and PathToDestination
// Now those functions can use their own instances of these variables
typedef struct PathingNodeReference
{
	PathingNode* pathingNode;
	struct PathingNodeReference* precedingNode;
	u16 g;
	u16 h;
	bool closed;
} PathingNodeReference;

// Used by Pathfinder_GetPath
int GetHeuristic(PathingNodeReference* source, PathingNode* destination)
{
	int xDist = abs(destination->posX - source->pathingNode->posX);
	int yDist = abs(destination->posY - source->pathingNode->posY);

	if (xDist > yDist)
	{
		return yDist * 14 + (xDist - yDist) * 10;
	}
	else
	{
		return xDist * 14 + (yDist - xDist) * 10;
	}
}

// Typical Astar search algorithm
bool Pathfinder_PathToDestination(Pathfinder* pathfinder, PathingNode* destination, Scenario* scenario)
{
	// Create a wrapper for each pathing node on the map, to store values that are specific to this algorithm
	PathingNodeReference* pathingNodes = malloc(scenario->mapSize * scenario->mapSize * sizeof(PathingNodeReference));

	for (int i = 0; i < scenario->mapSize * scenario->mapSize; i++)
	{
		pathingNodes[i].pathingNode = &scenario->pathingNodes[i];
		pathingNodes[i].g = 32767;
		pathingNodes[i].h = 0;
		pathingNodes[i].closed = false;
	}

	PathingNodeReference* openSet[512];

	int openSetLength = 1;

	openSet[0] = &pathingNodes[pathfinder->currentLocation->posX + pathfinder->currentLocation->posY * scenario->mapSize];

	openSet[0]->precedingNode = NULL;
	openSet[0]->g = 0;
	openSet[0]->h = GetHeuristic(openSet[0], destination);

	bool success = false;

	// While there are pathable neighbouring nodes to any nodes searched so far
	while (openSetLength > 0)
	{
		// currentNode = the neighbouring node with the lowest g + h cost
		PathingNodeReference* currentNode = openSet[0];
		int index = 0;

		for (int i = 1; i < openSetLength; i++)
		{
			if (openSet[i]->h < currentNode->h &&
				openSet[i]->h + openSet[i]->g < currentNode->h + currentNode->g)
			{
				currentNode = openSet[i];
				index = i;
			}
		}

		if (currentNode->pathingNode == destination)
		{
			success = true;
			break;
		}

		// Add any pathable neighbours to the open set, set their g, h costs
		for (int i = 0; i < currentNode->pathingNode->connectionCount; i++)
		{
			// Check if this connection is unpathable or already closed
			if ((currentNode->pathingNode->connections[i]->type & pathfinder->pathingFlag) == 0 ||
				currentNode->closed == true ||
				(currentNode->pathingNode->connections[i]->currentPathfinder != NULL &&
				!currentNode->pathingNode->connections[i]->currentPathfinder->isPathfinding))
			{
				continue;
			}

			int newG = currentNode->g + currentNode->pathingNode->connectionCosts[i];

			PathingNodeReference* connection = &pathingNodes[currentNode->pathingNode->connections[i]->posX + currentNode->pathingNode->connections[i]->posY * scenario->mapSize];

			if (newG < connection->g)
			{
				// If node isn't in open list
				// G value will always be 32767 if node hasn't been added to the open list yet
				if (connection->g == 32767)
				{
					openSet[openSetLength] = connection;
					openSetLength++;
				}

				connection->g = newG;
				connection->h = GetHeuristic(connection, destination);
				connection->precedingNode = currentNode;
			}
		}

		// Remove current element from open set
		openSetLength--;

		// memmove errors if it tries to move 0 bytes
		if (openSetLength > index)
		{
			// Shift all subsequent open set elements back by 1, overwriting the current element in the open set
			// Not overflowing because we already decremented openSetLength
			memmove(&openSet[index], &openSet[index + 1], sizeof(PathingNode*) * (openSetLength - index));
		}

		currentNode->closed = true;
	}

	// Remove old path
	if (pathfinder->currentPathLength > 0)
	{
		if (pathfinder->currentPath[pathfinder->currentPathLength - 1]->destinedPathfinder == pathfinder)
		{
			pathfinder->currentPath[pathfinder->currentPathLength - 1]->destinedPathfinder = NULL;
		}
	}

	if (pathfinder->currentPathLength > 0)
	{
		free(pathfinder->currentPath);
		pathfinder->currentPathLength = 0;
	}

	if (success)
	{
		// Get new path length
		int pathLength = 0;

		PathingNodeReference* pathLengthNode = &pathingNodes[destination->posX + destination->posY * scenario->mapSize];
		PathingNodeReference* destinationNode = pathLengthNode;

		if (pathLengthNode->pathingNode != pathfinder->currentLocation)
		{
			pathLength++;

			while (pathLengthNode->precedingNode->pathingNode != pathfinder->currentLocation)
			{
				pathLengthNode = pathLengthNode->precedingNode;
				pathLength++;
			}
		}

		// Assign new path
		pathfinder->currentPath = malloc(sizeof(PathingNode*) * pathLength);
		pathfinder->currentPathLength = pathLength;
		pathfinder->currentPathIndex = 0;

		for (int i = pathLength - 1; i >= 0; i--)
		{
			pathfinder->currentPath[i] = destinationNode->pathingNode;
			destinationNode = destinationNode->precedingNode;
		}
	}

	free(pathingNodes);

	return success;
}

// The path itself is generated by Pathfinder_PathToDestination
// The logic in this function serves as a proprocessor to find the closest pathable node to the destination
//
// This takes the form of a sort of dijkstra's algorithm,
// but targeting unpathable nodes instead of pathable ones, and without any backtracking for a path
//
// It also resolves cases where multiple pathfinders are targeting the same node
void Pathfinder_GetPath(Pathfinder* pathfinder, PathingNode* destination, Scenario* scenario)
{
	pathfinder->desiredLocation = destination;

	// Create a wrapper for each pathing node on the map, to store values that are specific to this algorithm
	PathingNodeReference* pathingNodes = malloc(scenario->mapSize * scenario->mapSize * sizeof(PathingNodeReference));

	for (int i = 0; i < scenario->mapSize * scenario->mapSize; i++)
	{
		pathingNodes[i].pathingNode = &scenario->pathingNodes[i];
		pathingNodes[i].g = 32767;
		pathingNodes[i].h = 0;
		pathingNodes[i].closed = false;
	}

	PathingNodeReference* openSet[512];

	int openSetLength = 1;

	openSet[0] = &pathingNodes[destination->posX + destination->posY * scenario->mapSize];

	openSet[0]->g = 0;

	// While there are unpathable neighbouring nodes to any nodes searched so far
	bool success = false;

	while (openSetLength > 0)
	{
		PathingNodeReference* currentNode;

		currentNode = openSet[0];
		int index = 0;

		for (int i = 1; i < openSetLength; i++)
		{
			if (openSet[i]->g < currentNode->g)
			{
				currentNode = openSet[i];
				index = i;
			}
		}

		bool pathable = false;

		if (pathfinder->pathingFlag == PATH_WATER)
		{
			if (currentNode->pathingNode->waterRegion == pathfinder->currentLocation->waterRegion)
			{
				pathable = true;
			}
		}
		else if (pathfinder->pathingFlag == PATH_LAND)
		{
			if (currentNode->pathingNode->landRegion == pathfinder->currentLocation->landRegion)
			{
				pathable = true;
			}
		}
		else
		{
			pathable = true;
		}

		if (pathable)
		{
			// Check if the pathable node already has a unit on it, and that unit isn't moving
			if (currentNode->pathingNode->currentPathfinder == NULL ||
				currentNode->pathingNode->currentPathfinder->isPathfinding ||
				currentNode->pathingNode->currentPathfinder == pathfinder)
			{
				if (Pathfinder_PathToDestination(pathfinder, currentNode->pathingNode, scenario))
				{
					// If the new destination happens to be where we already are, no more pathfinding
					if (currentNode->pathingNode->currentPathfinder == pathfinder)
					{
						pathfinder->isPathfinding = false;
					}

					// If there's already a pathfinder pathing toward this node
					if (currentNode->pathingNode->destinedPathfinder != NULL &&
						currentNode->pathingNode->destinedPathfinder != pathfinder)
					{
						// If current pathfinder is closer than the node's original pathfinder
						// TODO: same length && pathfinder index < destined index
						if (pathfinder->currentPathLength < currentNode->pathingNode->destinedPathfinder->currentPathLength - currentNode->pathingNode->destinedPathfinder->currentPathIndex)
						{
							Pathfinder* oldDestinedPathfinder = currentNode->pathingNode->destinedPathfinder;

							currentNode->pathingNode->destinedPathfinder = pathfinder;

							pathfinder = oldDestinedPathfinder;

							// If the original pathfinder wasn't targeting the same desired location as the current pathfinder
							if (oldDestinedPathfinder->desiredLocation != destination)
							{
								// Restart algorithm with the previously destined pathfinder, as they need a new location
								Pathfinder_GetPath(oldDestinedPathfinder, oldDestinedPathfinder->desiredLocation, scenario);
								return;
							}

							// Else switch the pathfinders, but continue with the algorithm as-is
						}

						// Else, continue with the algorithm
					}
					else
					{
						success = true;
						destination = currentNode->pathingNode;
						destination->destinedPathfinder = pathfinder;

						break;
					}
				}
			}
		}

		for (int i = 0; i < currentNode->pathingNode->connectionCount; i++)
		{
			// Check if this connection is already closed
			if (currentNode->closed == true)
			{
				continue;
			}

			int newG = currentNode->g + currentNode->pathingNode->connectionCosts[i];

			PathingNodeReference* connection = &pathingNodes[currentNode->pathingNode->connections[i]->posX + currentNode->pathingNode->connections[i]->posY * scenario->mapSize];

			if (newG < connection->g)
			{
				// If node isn't in open list
				// G value will always be 32767 if node hasn't been added to the open list yet
				if (connection->g == 32767)
				{
					openSet[openSetLength] = connection;
					openSetLength++;
				}

				connection->g = newG;
			}
		}

		// Remove current element from open set
		openSetLength--;

		// memmove errors if it tries to move 0 bytes
		if (openSetLength > index)
		{
			// Shift all subsequent open set elements back by 1, overwriting the current element in the open set
			// Not overflowing because we already decremented openSetLength
			memmove(&openSet[index], &openSet[index + 1], sizeof(PathingNode*) * (openSetLength - index));
		}

		currentNode->closed = true;
	}

	if (!success)
	{
		pathfinder->isPathfinding = false;
		return;
	}
}

void CommandPathfindersPoint(Pathfinder* pathfinders[], int pathfinderCount, PathingNode* destination, Scenario* scenario)
{
	// Clean up old paths so that pathfinders don't defer a destination to a pathfinder that's about to change theirs
	// Pre-emptively mark pathfinders as pathfinding so they don't act as pathing blockers
	for (int i = 0; i < pathfinderCount; i++)
	{
		if (pathfinders[i]->currentPathLength > 0)
		{
			pathfinders[i]->currentPath[pathfinders[i]->currentPathLength - 1]->destinedPathfinder = NULL;
		}

		pathfinders[i]->currentPathLength = 0;
		pathfinders[i]->currentPathIndex = 0;
		pathfinders[i]->isPathfinding = true;
	}

	for (int i = 0; i < pathfinderCount; i++)
	{
		Pathfinder_GetPath(pathfinders[i], destination, scenario);
	}
}

void CommandPathfindersFormation(Pathfinder* pathfinders[], int pathfinderCount, int offsetX, int offsetY, Scenario* scenario)
{
	// Clean up old paths so that pathfinders don't defer a destination to a pathfinder that's about to change theirs
	// Pre-emptively mark pathfinders as pathfinding so they don't act as pathing blockers
	for (int i = 0; i < pathfinderCount; i++)
	{
		if (pathfinders[i]->currentPathLength > 0)
		{
			pathfinders[i]->currentPath[pathfinders[i]->currentPathLength - 1]->destinedPathfinder = NULL;
		}

		pathfinders[i]->currentPathLength = 0;
		pathfinders[i]->currentPathIndex = 0;
		pathfinders[i]->isPathfinding = true;
	}

	for (int i = 0; i < pathfinderCount; i++)
	{
		int indexX = pathfinders[i]->currentLocation->posX + offsetX;
		int indexY = pathfinders[i]->currentLocation->posY + offsetY;

		if (indexX < 0)
		{
			indexX = 0;
		}
		else if (indexX >= scenario->mapSize)
		{
			indexX = scenario->mapSize - 1;
		}

		if (indexY < 0)
		{
			indexY = 0;
		}
		else if (indexY >= scenario->mapSize)
		{
			indexY = scenario->mapSize - 1;
		}

		PathingNode* node = &scenario->pathingNodes[indexX + indexY * scenario->mapSize];
		Pathfinder_GetPath(pathfinders[i], node, scenario);
	}
}

void Pathfinder_Move(Pathfinder* pathfinder, Pathfinder* dependencyStack[], int dependencyStackLength, Scenario* scenario)
{
	if (pathfinder->isPathfinding)
	{
		bool recalculatePath = false;

		for (int i = pathfinder->currentPathIndex; i < pathfinder->currentPathLength; i++)
		{
			if ((pathfinder->currentPath[i]->type & pathfinder->pathingFlag) == 0 ||
				(pathfinder->currentPath[i]->currentPathfinder != NULL &&
				!pathfinder->currentPath[i]->currentPathfinder->isPathfinding))
			{
				recalculatePath = true;
				break;
			}
		}

		if (recalculatePath)
		{
			Pathfinder_GetPath(pathfinder, pathfinder->desiredLocation, scenario);

			// It's possible that the new destination will be where we already are
			// Need to not move in this case
			if (!pathfinder->isPathfinding)
			{
				return;
			}
		}

		Pathfinder* nextNodePathfinder = pathfinder->currentPath[pathfinder->currentPathIndex]->currentPathfinder;

		// If next node is blocked by another pathfinder
		if (nextNodePathfinder != NULL)
		{
			// If the blocker hasn't moved yet, try adding them to the stack
			if (!nextNodePathfinder->hasMoved)
			{
				bool circularDependency = false;

				// Check for a circular dependency
				for (int i = 0; i < dependencyStackLength; i++)
				{
					if (nextNodePathfinder == dependencyStack[i])
					{
						// Circular Dependency
						// Resolve all pathfinders down to nextNodePathfinder
						// Discard the rest
						circularDependency = true;

						dependencyStack[dependencyStackLength] = pathfinder;

						bool finished = false;

						while (true)
						{
							pathfinder = dependencyStack[dependencyStackLength];

							pathfinder->currentLocation = pathfinder->currentPath[pathfinder->currentPathIndex];

							pathfinder->currentLocation->currentPathfinder = pathfinder;

							// If reached destination
							if (pathfinder->currentLocation->destinedPathfinder == pathfinder)
							{
								pathfinder->currentLocation->destinedPathfinder = NULL;
								pathfinder->isPathfinding = false;
							}

							pathfinder->currentPathIndex++;
							pathfinder->hasMoved = true;

							dependencyStackLength--;

							// Using this because it needs to run one last time once the end condition is met
							if (finished)
							{
								break;
							}

							if (dependencyStack[dependencyStackLength] == nextNodePathfinder)
							{
								finished = true;
							}
						}

						// Check if any pathfinders were in the stack before the circular dependency
						//
						// dependencyStackLength is currently off by one
						// The most recent pathfinder was added in place of dependencyStackLength
						// And dependencyStackLength was never incremented
						if (dependencyStackLength > -1)
						{
							pathfinder = dependencyStack[dependencyStackLength];

							// Check if the pathfinder at the beginning of the circular dependency is now stationary
							if (!pathfinder->currentPath[pathfinder->currentPathIndex]->currentPathfinder->isPathfinding)
							{
								dependencyStackLength--;

								Pathfinder_Move(pathfinder, dependencyStack, dependencyStackLength, scenario);
							}
						}

						return;
					}
				}

				if (!circularDependency)
				{
					dependencyStack[dependencyStackLength] = pathfinder;
					dependencyStackLength++;

					if (dependencyStackLength > 128)
					{
						printf("DEPENDENCY STACK LIMIT BREACHED >128\n");
					}

					Pathfinder_Move(pathfinder->currentPath[pathfinder->currentPathIndex]->currentPathfinder, dependencyStack, dependencyStackLength, scenario);

					return;
				}
			}
			else
			{
				// Reset dependency stack, blocked so no action
				dependencyStackLength = 0;
			}
		}
		else
		{
			// Add to dependency stack, then move all the pathfinders in the stack, since they're all free now
			dependencyStack[dependencyStackLength] = pathfinder;

			while (dependencyStackLength >= 0)
			{
				pathfinder = dependencyStack[dependencyStackLength];

				pathfinder->currentLocation->currentPathfinder = NULL;

				pathfinder->currentLocation = pathfinder->currentPath[pathfinder->currentPathIndex];

				pathfinder->currentLocation->currentPathfinder = pathfinder;

				// If reached destination
				if (pathfinder->currentLocation->destinedPathfinder == pathfinder)
				{
					pathfinder->currentLocation->destinedPathfinder = NULL;
					pathfinder->isPathfinding = false;
				}

				pathfinder->currentPathIndex++;
				pathfinder->hasMoved = true;

				dependencyStackLength--;
			}
		}
	}
}

// Used by Scenario_UpdateRegions
// NOT TESTED ON WATER YET @@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@
void FloodFill(PathingNode* startingNode, int region, bool water)
{
	// Get the address of startingNode.landRegion
	// If water == 1, add 1 byte to the address
	// Set the data of the resulting address, now either pointing to landRegion or waterRegion
	*(&startingNode->landRegion + water) = region;

	for (int i = 0; i < startingNode->connectionCount; i++)
	{
		// Part 1:
		// 00000001 refers to land pathability, so we compare against 0b00000001
		// 00000010 refers to water pathability
		// if water is true, (startingNode->connections[i]->type >> water) turns 00000010 into 00000001
		// Now both land and water are determined by 00000001
		// This allows for a simple comparison
		//
		// Part 2:
		// Perform the same address arithmetic from earlier, but on the connected node to check if it's already been set
		if (((startingNode->connections[i]->type >> water) & 0b00000001) == 0b00000001 &&
			*(&startingNode->connections[i]->landRegion + water) == 0)
		{
			FloodFill(startingNode->connections[i], region, water);
		}
	}
}

void Scenario_UpdateRegions(Scenario* scenario)
{
	for (int i = 0; i < scenario->mapSize * scenario->mapSize; i++)
	{
		scenario->pathingNodes[i].landRegion = 0;
		scenario->pathingNodes[i].waterRegion = 0;
	}

	int currentRegion = 1;

	for (int i = 0; i < scenario->mapSize * scenario->mapSize; i++)
	{
		if (scenario->pathingNodes[i].landRegion == 0 &&
			scenario->pathingNodes[i].type & PATH_LAND)
		{
			FloodFill(&scenario->pathingNodes[i], currentRegion, false);
			currentRegion++;
		}
	}

	currentRegion = 1;

	for (int i = 0; i < scenario->mapSize * scenario->mapSize; i++)
	{
		if (scenario->pathingNodes[i].waterRegion == 0 &&
			scenario->pathingNodes[i].type & PATH_WATER)
		{
			FloodFill(&scenario->pathingNodes[i], currentRegion, true);
			currentRegion++;
		}
	}
}

#endif
