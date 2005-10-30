
#include "rng.h"
#include "debug.h"

#include <ctime>
#include <cmath>

using namespace std;

static Rng desyncrng(time(NULL));

void Rng::sfrand(int in_seed) {
    dprintf("Seeding random generator with %d\n", in_seed);
    seed = in_seed;
    sync = seed;
    frand(); frand();
    start = true;
}

int Rng::rand() {
	sync = sync * 1103515245 + 12345;
	return ((sync >>16) & 32767);
}

float Rng::frand() {
    start = false;
    return (float)rand() / 32768;
}

int Rng::frandseed() {
    CHECK(start);
    return seed;
}

Rng::Rng() {
    sync = 1;
    seed = 1;
    start = false;
    sfrand(desyncrng.rand());
}

Rng::Rng(int in_seed) {
    sync = 1;
    seed = 1;
    start = false;
    sfrand(in_seed);
}

static Rng syncrng;

void sfrand(int seed) { syncrng.sfrand(seed); };
float frand() { return syncrng.frand(); };
int frandseed() { return syncrng.frandseed(); };

float powerRand(float pw) {
    float stp = pow(frand(), pw);
    if(frand() < 0.5)
        stp = -stp;
    return stp;
}
