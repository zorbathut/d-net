#ifndef DNET_CFC
#define DNET_CFC

#include "float.h"
using namespace std;


class Coord;
class Coord2;
class Coord4;

class CFC {
public:
  float v;

  float &operator*() { return v; };
  float operator*() const { return v; };
  float *operator->() { return &v; };
  const float *operator->() const { return &v; };

  CFC();
  CFC(const CFC &cf) {
    v = cf.v;
  }
  CFC(const float &f) {
    v = f;
  }
  CFC(const Coord &c);
};

class CFC2 {
public:
  Float2 v;

  Float2 &operator*() { return v; };
  const Float2 &operator*() const { return v; };
  Float2 *operator->() { return &v; };
  const Float2 *operator->() const { return &v; };

  CFC2();
  CFC2(const CFC2 &cf) {
    v = cf.v;
  }
  CFC2(const Float2 &f) {
    v = f;
  }
  CFC2(const Coord2 &c);
};

class CFC4 {
public:
  Float4 v;

  Float4 &operator*() { return v; };
  const Float4 &operator*() const { return v; };
  Float4 *operator->() { return &v; };
  const Float4 *operator->() const { return &v; };

  CFC4();
  CFC4(const CFC4 &cf) {
    v = cf.v;
  }
  CFC4(const Float4 &f) {
    v = f;
  }
  CFC4(const Coord4 &c);
};

#endif
