
#include "rng.h"

static unsigned int sync = 1;
static unsigned int unsync = 1;

int syncRand( ) {
	sync = sync * 1103515245 + 12345;
	return ((sync >>16) & 32767);
}

int unsyncRand( ) {
	unsync = unsync * 1103515245 + 12345;
	return ((unsync >>16) & 32767);
}
