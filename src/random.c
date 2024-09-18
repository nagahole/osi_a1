#include <stdlib.h>

// technically not 100% uniform (assuming rand() itself is uniform)
// but practically satisfactory
int randint(int min, int max)
{
    if (max <= min)
    {
        return min;
    }

    return min + (rand() % (max - min));
}
