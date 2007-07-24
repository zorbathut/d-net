#ifndef DNET_RNG
#define DNET_RNG

class RngSeed {
friend class Rng;
private:
  int seed;

public:
  explicit RngSeed(int seed) : seed(seed) { }
};

class Rng {
private:
  unsigned int sync;

  Rng();

public:
  float frand(); // [0,1)
  float sym_frand(); // (-1,1)

  float gaussian(); // returns gaussian distribution with a standard deviation of 1
  float gaussian(float maxgauss);  // returns gaussian distribution with a standard deviation of 1, maximum deviation of max (just chops off the probability curve)
  float gaussian_scaled(float maxgauss); // returns gaussian(maxgauss) / maxgauss

  RngSeed generate_seed();

  explicit Rng(RngSeed seed);
};

Rng &unsync();

#endif
