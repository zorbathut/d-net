
#include "rng.h"

#include "debug.h"
#include "coord.h"
#include "adler32.h"

#include <cmath>

using namespace std;

float Rng::frand() {
  return sync();
}

// todo: something more cleverer
float Rng::sym_frand() {
  if(frand() < 0.5)
    return frand();
  else
    return -frand();
}

float Rng::gaussian() {
  float x1, x2, w, y1;
  
  do {
    x1 = 2.0 * frand() - 1.0;
    x2 = 2.0 * frand() - 1.0;
    w = x1 * x1 + x2 * x2;
  } while (w >= 1.0);
  
  w = sqrt((-2.0 * log(w)) / w);
  y1 = x1 * w;
  
  return y1;
}

int Rng::choose(int choices) {
  return int(frand() * choices);
}

float Rng::gaussian(float maxgauss) {
  while(1) {
    float gauss = gaussian();
    if(abs(gauss) <= maxgauss)
      return gauss;
  }
}

float Rng::gaussian_scaled(float maxgauss) {
  return gaussian(maxgauss) / maxgauss;
}

Coord Rng::cfrand() {
  return Coord(sync());
}

Coord Rng::sym_cfrand() {
  if(frand() < 0.5)
    return cfrand();
  else
    return -cfrand();
}

Coord Rng::cgaussian() {
  Coord x1, x2, w, y1;
  
  do {
    x1 = 2 * cfrand() - 1;
    x2 = 2 * cfrand() - 1;
    w = x1 * x1 + x2 * x2;
  } while (w >= 1);
  
  w = sqrt((-2 * log(w)) / w);
  y1 = x1 * w;
  
  return y1;
}

Coord Rng::cgaussian(Coord maxgauss) {
  while(1) {
    Coord gauss = cgaussian();
    if(abs(gauss) <= maxgauss)
      return gauss;
  }
}

Coord Rng::cgaussian_scaled(Coord maxgauss) {
  return cgaussian(maxgauss) / maxgauss;
}

RngSeed Rng::generate_seed() {
  RngSeed rv(0);
  do {
    rv = RngSeed((unsigned int)(frand() * (1LL << 32)));
  } while(rv.seed == 0);
  return rv;
}

void Rng::checksum(Adler32 *adl) const {
  Rng trng = *this;
  adler(adl, trng.cfrand());
  adler(adl, trng.cfrand());
  adler(adl, trng.cfrand());
  adler(adl, trng.cfrand());
}

Rng::Rng(RngSeed in_seed) {
  sync = boost::lagged_fibonacci9689(in_seed.seed);
}

Rng &unsync() { static Rng unsyncrng(RngSeed(time(NULL))); return unsyncrng; }
