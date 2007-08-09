
#include "adler32.h"

#include "debug.h"
#include "coord.h"

#include <boost/static_assert.hpp>

Adler32::Adler32() {
  a = 1;
  b = 0;
}

unsigned long Adler32::output() const {
  CHECK(a < 65521);
  CHECK(b < 65521);
  return b * 65536 + a;
}

void Adler32::addByte(unsigned char x) {
  b += a += x;
  a = a % 65521;
  b = b % 65521;
}
void Adler32::addBytes(const void *x, int len) {
  CHECK(len < 5550); // heh.
  const unsigned char *dv = (const unsigned char *)x;
  for(int i = 0; i < len; i++)
    b += a += *dv++;
  a = a % 65521;
  b = b % 65521;
}

BOOST_STATIC_ASSERT(sizeof(int) == 4);
BOOST_STATIC_ASSERT(sizeof(long long) == 8);
BOOST_STATIC_ASSERT(sizeof(Coord) == 8);
BOOST_STATIC_ASSERT(sizeof(Coord2) == 16);
BOOST_STATIC_ASSERT(sizeof(Coord4) == 32);

void adler(Adler32 *adl, int val) {
  adl->addBytes(&val, sizeof(val));
}
void adler(Adler32 *adl, const Coord &val) {
  adl->addBytes(&val, sizeof(val));
}
void adler(Adler32 *adl, const Coord2 &val) {
  adl->addBytes(&val, sizeof(val));
}
void adler(Adler32 *adl, const Coord4 &val) {
  adl->addBytes(&val, sizeof(val));
}
