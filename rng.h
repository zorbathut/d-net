#ifndef DNET_RNG
#define DNET_RNG

#include <boost/random.hpp>

class Coord;
class Adler32;
  
class RngSeed {
public:
  unsigned int seed;

public:
  explicit RngSeed(unsigned int seed) : seed(seed) { }
};

template<typename T> class RngBase {
private:
  T sync;

  RngBase();

public:
  float frand(); // [0,1)
  float sym_frand(); // (-1,1)

  int choose(int opts);

  float gaussian(); // returns gaussian distribution with a standard deviation of 1
  float gaussian(float maxgauss);  // returns gaussian distribution with a standard deviation of 1, maximum deviation of max (just chops off the probability curve)
  float gaussian_scaled(float maxgauss); // returns gaussian(maxgauss) / maxgauss

  Coord cfrand(); // [0,1)
  Coord sym_cfrand(); // (-1,1)

  Coord cgaussian(); // returns gaussian distribution with a standard deviation of 1
  Coord cgaussian(Coord maxgauss);  // returns gaussian distribution with a standard deviation of 1, maximum deviation of max (just chops off the probability curve)
  Coord cgaussian_scaled(Coord maxgauss); // returns gaussian(maxgauss) / maxgauss

  RngSeed generate_seed();

  void checksum(Adler32 *adler) const;

  explicit RngBase(RngSeed seed);
};

typedef RngBase<boost::lagged_fibonacci9689> Rng;
typedef RngBase<boost::rand48> RngFast;

Rng &unsync();

inline void adler(Adler32 *adler, const Rng &rng) { rng.checksum(adler); }

#endif
