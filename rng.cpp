
#include "rng.h"
#include "debug.h"

static unsigned int sync = 1;
static int seed = 1;
static bool start = false;

void sfrand(int in_seed) {
    seed = in_seed;
    sync = seed;
    frand(); frand();
    start = true;
}

int syncRand( ) {
	sync = sync * 1103515245 + 12345;
	return ((sync >>16) & 32767);
}

float frand() {
    start = false;
    return (float)syncRand() / 32768;
}

int frandseed() {
    CHECK(start);
    return seed;
}
