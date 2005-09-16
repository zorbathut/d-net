
#include "rng.h"

static unsigned int sync = 1;

void sfrand(int seed) {
    sync = seed;
    frand(); frand();
}

int syncRand( ) {
	sync = sync * 1103515245 + 12345;
	return ((sync >>16) & 32767);
}

float frand() {
    return (float)syncRand() / 32768;
}
