
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
BOOST_STATIC_ASSERT(sizeof(long long) == 8);
BOOST_STATIC_ASSERT(sizeof(Coord) == 8);
BOOST_STATIC_ASSERT(sizeof(Coord2) == 16);
BOOST_STATIC_ASSERT(sizeof(Coord4) == 32);

void adler(Adler32 *adl, const Coord &val) { adl->addBytes(&val, sizeof(val)); }
void adler(Adler32 *adl, const Coord2 &val) { adl->addBytes(&val, sizeof(val)); }
void adler(Adler32 *adl, const Coord4 &val) { adl->addBytes(&val, sizeof(val)); }


static bool read = false;
static int wpos = 0;
static int rpos = 0;
static vector<unsigned long> adli;

void reg_adler_ul_worker(unsigned long dat, const char *file, int line) {
  if(read) {
    if(wpos && adli[wpos - 1] == dat)
      return;
    if(wpos >= adli.size()) {
      dprintf("Too many adler items at %s:%d frame %d (%d valid)\n", file, line, frameNumber, adli.size());
      CHECK(0);
    }
    if(adli[wpos] != dat) {
      dprintf("Adler consistency failure at %s:%d %d (%08lx vs %08lx)\n", file, line, frameNumber, adli[wpos], dat);
      CHECK(0);
    }
    wpos++;
  } else {
    if(!adli.size() || adli.back() != dat)
      adli.push_back(dat);
  }
}

void reg_adler_ref_start() {
  read = true;
  adli.clear();
  wpos = 0;
  rpos = 0;
}
void reg_adler_ref_item(unsigned long unl) {
  CHECK(read);
  adli.push_back(unl);
}

int ret_adler_ref_count() {
  return adli.size();
}
unsigned long ret_adler_ref() {
  return adli[rpos++];
}
void ret_adler_ref_clear() {
  adli.clear();
  wpos = 0;
  rpos = 0;
}
