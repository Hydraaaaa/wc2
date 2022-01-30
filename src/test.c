#include "pathfinder.c"

#define ACTION_SELECT        1
#define ACTION_PATHFIND      2
#define ACTION_MOVE          3
#define ACTION_SPAWN_GRUNT   4
#define ACTION_SPAWN_BLOCKER 5

typedef struct
{
	u8 x;
	u8 y;
} Point;

typedef struct TestFrameData TestFrameData;
typedef struct TestAction TestAction;


struct TestFrameData
{
	TestFrameData* next;

	Point* pathfinderPositions;
	Point** pathfinderPaths;
	u16* pathfinderPathLengths;

	u16 pathfinderCount;
};

struct TestAction
{
	TestAction* next;

	// 1 = Select(minX, minY, maxX, maxY) N
	// 2 = Pathfind(x, y)                 Y
	// 3 = Move()                         Y
	// 4 = SpawnGrunt(x, y)               N
	// 5 = SpawnBlocker(x, y)             N
	u8 action;

	u8 param1;
	u8 param2;
	u8 param3;
	u8 param4;
};

typedef struct
{
	TestAction* firstAction;
	TestAction* lastAction;
	TestFrameData* firstFrameData;
	TestFrameData* lastFrameData;
} Test;

void Test_Init(Test* test)
{
	test->firstAction = NULL;
	test->lastAction = NULL;
	test->firstFrameData = NULL;
	test->lastFrameData = NULL;
}

void Test_AllocateAction(Test* test)
{
	if (test->lastAction == NULL)
	{
		test->firstAction = malloc(sizeof(TestAction));
		test->lastAction = test->firstAction;
	}
	else
	{
		TestAction* lastAction = test->lastAction;

		test->lastAction = malloc(sizeof(TestAction));

		test->lastAction->next = NULL;

		lastAction->next = test->lastAction;
	}
}

void Test_AllocateFrameData(Test* test)
{
	if (test->lastFrameData == NULL)
	{
		test->firstFrameData = malloc(sizeof(TestFrameData));
		test->lastFrameData = test->firstFrameData;
	}
	else
	{
		TestFrameData* lastFrameData = test->lastFrameData;

		test->lastFrameData = malloc(sizeof(TestFrameData));

		test->lastFrameData->next = NULL;

		lastFrameData->next = test->lastFrameData;
	}
}

void TestFrameData_Populate(TestFrameData* frameData, Pathfinder* pathfinders, int pathfinderCount)
{
	frameData->pathfinderCount = pathfinderCount;

	frameData->pathfinderPositions = malloc(sizeof(Point) * pathfinderCount);
	frameData->pathfinderPaths = malloc(sizeof(Point*) * pathfinderCount);
	frameData->pathfinderPathLengths = malloc(sizeof(u16) * pathfinderCount);

	for (int i = 0; i < pathfinderCount; i++)
	{
		frameData->pathfinderPositions[i].x = pathfinders[i].currentLocation->posX;
		frameData->pathfinderPositions[i].y = pathfinders[i].currentLocation->posY;

		frameData->pathfinderPaths[i] = malloc(sizeof(Point) * pathfinders[i].currentPathLength);
		frameData->pathfinderPathLengths[i] = pathfinders[i].currentPathLength;

		for (int j = 0; j < pathfinders[i].currentPathLength; j++)
		{
			frameData->pathfinderPaths[i][j].x = pathfinders[i].currentPath[j]->posX;
			frameData->pathfinderPaths[i][j].y = pathfinders[i].currentPath[j]->posY;
		}
	}
}
