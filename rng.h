#ifndef DNET_RNG
#define DNET_RNG

class Rng {
private:
    unsigned int sync;
    int seed;
    bool start;
public:
    void sfrand(int seed);
    int rand();
    float frand();
    int frandseed();

    Rng();
    Rng(int seed);
};

void sfrand(int seed);
float frand();
int frandseed(); // only valid until frand() is called

#endif
