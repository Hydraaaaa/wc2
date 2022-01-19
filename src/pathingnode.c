#ifndef PATHINGNODE_C
#define PATHINGNODE_C

#include "pathfinder.h"

struct PathingNode
{
	struct PathingNode* connections[8];
	Pathfinder* currentPathfinder;
	Pathfinder* destinedPathfinder; // Pathfinder that's eventually expected to reach this node and stop pathing
	u8 connectionCosts[8];
	u8 connectionCount;

	u8 posX;
	u8 posY;

	// Type
	// 00000001 Land Pathable
	// 00000010 Water Pathable
	// 00000100 Air Pathable
	u8 type;

	// Regions are areas of the pathfinding grid that are entirely sealed off from eachother
	// Comparing against a region is much cheaper than failing an entire pathing check
	u8 landRegion;  // |
	u8 waterRegion; // | These two must stay in this order, address arithmetic is performed accordingly
};

#endif
