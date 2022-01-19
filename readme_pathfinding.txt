PATHFINDING

Upon being commanded to a location
1. If the pathfinder already has a path to a location
	Clear that path
2. Mark pathfinder as pathfinding // Do this for all commanded pathfinders before step 3
3. Find nearest destination to target location
4. Path toward available destination
	If encountering any pathfinders marked as pathfinding
	Treat them as a free space
5. Check if other pathfinder is pathing toward same destination
6. Check if other pathfinder is closer to the destination (in pathfinding steps)
	If so, continue with the algorithm of 1
	Else, check if other pathfinder has the same target location
		If so, continue with algorithm of 1, but switching the target pathfinder to the other one
		Else, Run new instance of algorithm of 1, with this other pathfinder

Let's say we have one pathfinder P, ordered to the destination X

########
#     X#
#      #
#P     #
########

This should simply perform a pathfind to X

Let's say we have two Ps ordered to X

########
###   X#
#PP    #
###    #
########

Let's say the first P is the one on the left, it's effectively trapped
This is resolved by first marking all ordered Ps as pathfinding, even before they have a path
Then comparing against that flag to see if a P should act as an obstacle in their path

Once the first P has pathed to X, the second one does as well, only to find that there's already a P targeting that tile
It then compares the length of its path against the length of the other P's path
Upon finding that it's closer to X, it takes priority and paths to X, while then making the first P search for the nearest available node to X

The result may look something like this

########
###    #
#P....X#
###    #
########

########
###  .X#
# P..  #
###    #
########

Once networking enters the equation, each pathfinder may need to do its pathfinding right before the movement tick to ensure deterministic behaviour with the order of units pathing

MOVING
(This is all theoretical at the moment)

Both Ps have their corresponding paths based on X
Now, they actually have to move

########
###   X#
#PP    #
###    #
########

The leftmost P has the first move, but notices that the other P is in the way
The first P then defers to the second P to perform its move first

########
###   X#
#P P   #
###    #
########

After this move has taken place, we can then return to the first P to take its move

########
###   X#
# PP   #
###    #
########

If we continue this to the end, this is what happens

########  ########  ########  ########  ########  ########  ########
###   X#  ###   X#  ###  PX#  ###  PX#  ###  PX#  ###   P#  ###   P#
# P P  #  #  PP  #  #  P   #  #   P  #  #    P #  #    P #  #     P#
###    #  ###    #  ###    #  ###    #  ###    #  ###    #  ###    #
########  ########  ########  ########  ########  ########  ########

Now let's say that we have two Ps, the left P targeting 1, and the right P targeting 2

########
#      #
#   P  #
#P..2.1#
########

########
#      #
#      #
# P.P.1#
########

After a move, the right P is no longer pathfinding and is now static
The left P checks before each move to see if a static blocker has appeared anywhere on its path
If it has, P then performs a new pathfind on its destination, and finds another way around

########  ########  ########  ########  ########
#      #  #      #  #      #  #      #  #      #
#  ... #  #  P.. #  #   P. #  #    P #  #      #
# P P 1#  #   P 1#  #   P 1#  #   P 1#  #   P P#
########  ########  ########  ########  ########

Circular Dependency

#####
#PPP#
#P P#
#PPP#
#####

All Ps here are targeting the position of the P clockwise from it

In this instance, whichever P goes first will end up deferring its movement until after the movement of the next clockwise P
This will lead to a stack of deferrals
This stack of deferrals is kept track of, and each P will check if the P it defers to is already in the stack
In the case of it already being in the stack, each P in the stack will then proceed with their movement and resolve the stack

Numbering the Ps, this is what happens

#####  #####
#123#  #812#
#8 4#  #7 3#
#765#  #654#
#####  #####
