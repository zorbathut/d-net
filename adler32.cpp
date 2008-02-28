
#include "adler32.h"

#include "debug.h"
#include "coord.h"

#include <boost/static_assert.hpp>

Adler32::Adler32() {
  a = 1;
  b = 0;
  
  count = 0;
}

unsigned long Adler32::output() const {
  unsigned int ta = a % MOD_ADLER;
  unsigned int tb = b % MOD_ADLER;
  return (tb << 16) + ta;
}

BOOST_STATIC_ASSERT(sizeof(int) == 4);
BOOST_STATIC_ASSERT(sizeof(long) == 4);
BOOST_STATIC_ASSERT(sizeof(long long) == 8);
