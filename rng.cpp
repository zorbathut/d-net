
#include "rng.h"

#include "debug.h"
#include "coord.h"
#include "adler32.h"

#include <cmath>

using namespace std;

template<> float RngBase<boost::lagged_fibonacci9689>::frand() {
  float v = (float)sync();
  if(unlikely(v == 1.0))
    return frand();
  return v;
}

template<> float RngBase<boost::rand48>::frand() {
  float v = (float)sync() / boost::rand48::max_value;
  if(unlikely(v == 1.0))
    return frand();
  return v;
}

// todo: something more cleverer
template<typename T> float RngBase<T>::sym_frand() {
  if(frand() < 0.5)
    return frand();
  else
    return -frand();
}

template<typename T> float RngBase<T>::gaussian() {
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

template<typename T> int RngBase<T>::choose(int choices) {
  return int(frand() * choices);
}

template<typename T> float RngBase<T>::gaussian(float maxgauss) {
  while(1) {
    float gauss = gaussian();
    if(abs(gauss) <= maxgauss)
      return gauss;
  }
}

template<typename T> float RngBase<T>::gaussian_scaled(float maxgauss) {
  return gaussian(maxgauss) / maxgauss;
}

template<typename T> Coord RngBase<T>::cfrand() {
  return Coord(frand());
}

template<typename T> Coord RngBase<T>::sym_cfrand() {
  if(frand() < 0.5)
    return cfrand();
  else
    return -cfrand();
}

template<typename T> Coord RngBase<T>::cgaussian() {
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

template<typename T> Coord RngBase<T>::cgaussian(Coord maxgauss) {
  while(1) {
    Coord gauss = cgaussian();
    if(abs(gauss) <= maxgauss)
      return gauss;
  }
}

template<typename T> Coord RngBase<T>::cgaussian_scaled(Coord maxgauss) {
  return cgaussian(maxgauss) / maxgauss;
}

template<typename T> RngSeed RngBase<T>::generate_seed() {
  RngSeed rv(0);
  do {
    rv = RngSeed((unsigned int)(frand() * (1LL << 32)));
  } while(rv.getSeed() == 0);
  return rv;
}

template<typename T> void RngBase<T>::checksum(Adler32 *adl) const {
  RngBase<T> trng = *this;
  adler(adl, trng.cfrand());
  adler(adl, trng.cfrand());
  adler(adl, trng.cfrand());
  adler(adl, trng.cfrand());
}

template<typename T> RngBase<T>::RngBase(RngSeed in_seed) {
  sync = T((int)in_seed.getSeed());
}

Rng &unsync() { static Rng unsyncrng(RngSeed(time(NULL))); return unsyncrng; }

void dump() {
  {
    Rng rng(RngSeed(time(NULL)));
    rng.frand();
    rng.cfrand();
    rng.sym_frand();
    rng.gaussian();
    rng.gaussian_scaled(1);
    rng.cgaussian_scaled(1);
    rng.generate_seed();
    rng.choose(15);
    rng.checksum(NULL); // lol
  }
  
  {
    RngFast rng(RngSeed(time(NULL)));
    rng.frand();
    rng.cfrand();
    rng.sym_frand();
    rng.gaussian();
    rng.gaussian_scaled(1);
    rng.cgaussian_scaled(1);
    rng.generate_seed();
    rng.choose(15);
    rng.checksum(NULL); // lol
  }
}
