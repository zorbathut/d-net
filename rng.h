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

float gaussian(); // returns gaussian distribution with a standard deviation of 1
float gaussian(float maxgauss);  // returns gaussian distribution with a standard deviation of 1, maximum deviation of max (just chops off the probability curve)
float gaussian_scaled(float maxgauss); // returns gaussian(maxgauss) / maxgauss

float unsync_frand(); // [0,1)
float unsync_symfrand();  // (-1,1)

#endif
