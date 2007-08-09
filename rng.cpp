
#include "rng.h"

#include "debug.h"
#include "coord.h"

#include <cmath>

using namespace std;

float Rng::frand() {
  sync = sync * 1103515245 + 12345;
  return (float)((sync >> 16) & 32767) / 32768;
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
  sync = sync * 1103515245 + 12345;
  return Coord((sync >> 16) & 32767) / 32768;
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
  return RngSeed((int)(frand() * (1LL << 32)));
}

void Rng::checksum(Adler32 *adl) const {
  adler(adl, sync);
}

Rng::Rng(RngSeed in_seed) {
  sync = in_seed.seed;
  frand(); frand();
}

static Rng unsyncrng(RngSeed(time(NULL)));

Rng &unsync() { return unsyncrng; }
