
#include "rng.h"

#include "debug.h"

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

RngSeed Rng::generate_seed() {
  return RngSeed((int)(frand() * (1LL << 32)));
}

Rng::Rng(RngSeed in_seed) {
  sync = in_seed.seed;
  frand(); frand();
}

static Rng unsyncrng(RngSeed(time(NULL)));

Rng &unsync() { return unsyncrng; }
