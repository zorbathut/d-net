
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

static vector<const AdlerIgnore *> aigl;

AdlerIgnore::AdlerIgnore() {
  aigl.push_back(this);
}
AdlerIgnore::~AdlerIgnore() {
  CHECK(aigl.size());
  CHECK(aigl.back() == this);
  aigl.pop_back();
}

enum AdlerState { ADLER_IDLE, ADLER_COMPARE_PRIMING, ADLER_COMPARING, ADLER_CREATING, ADLER_COMPARING_PAUSED, ADLER_CREATING_PAUSED, ADLER_OUTPUTTING } state;

static int currentpos = 0;
static vector<unsigned long> adli;

void reg_adler_start_create() {
  CHECK(state == ADLER_IDLE);
  adli.clear();
  state = ADLER_CREATING;
}
void reg_adler_start_compare() {
  CHECK(state == ADLER_IDLE);
  adli.clear();
  state = ADLER_COMPARE_PRIMING;
}

void reg_adler_start_compare_add(unsigned long unl) {
  CHECK(state == ADLER_COMPARE_PRIMING);
  adli.push_back(unl);
}
void reg_adler_start_compare_done() {
  CHECK(state == ADLER_COMPARE_PRIMING);
  state = ADLER_COMPARING;
  currentpos = 0;
}

void reg_adler_ul_data(unsigned long dat, const char *file, int line, const char *msg) {
  if(aigl.size()) {
  } else if(state == ADLER_COMPARING) {
    if(currentpos && adli[currentpos - 1] == dat)
      return;
    if(currentpos >= adli.size()) {
      dprintf("Too many adler items at %s:%d frame %d (%d valid)\n", file, line, frameNumber, adli.size());
      CHECK(0);
    }
    if(adli[currentpos] != dat) {
      dprintf("Adler consistency failure at %s:%d %d (%08lx vs %08lx)\n", file, line, frameNumber, adli[currentpos], dat);
      if(msg)
        dprintf("%s\n", msg);
      CHECK(0);
    }
    currentpos++;
  } else if(state == ADLER_CREATING) {
    if(!adli.size() || adli.back() != dat)
      adli.push_back(dat);
  } else {
    CHECK(0);
  }
}

void reg_adler_pause() {
  if(state == ADLER_COMPARING) {
    state = ADLER_COMPARING_PAUSED;
  } else if(state == ADLER_CREATING) {
    state = ADLER_CREATING_PAUSED;
  } else {
    CHECK(0);
  }
}

void reg_adler_unpause() {
  if(state == ADLER_COMPARING_PAUSED) {
    state = ADLER_COMPARING;
  } else if(state == ADLER_CREATING_PAUSED) {
    state = ADLER_CREATING;
  } else {
    CHECK(0);
  }
}

void reg_adler_register_finished() {
  if(state == ADLER_COMPARING) {
    if(currentpos != adli.size()) {
      dprintf("%d, %d\n", currentpos, adli.size());
      CHECK(currentpos == adli.size());
    }
  } else if(state == ADLER_CREATING) {
  } else {
    CHECK(0);
  }
  state = ADLER_OUTPUTTING;
  currentpos = 0;
}

int reg_adler_read_count() {
  CHECK(state == ADLER_OUTPUTTING || state == ADLER_IDLE);
  return adli.size();
}
unsigned long reg_adler_read_ref() {
  CHECK(state == ADLER_OUTPUTTING);
  CHECK(currentpos <= adli.size());
  return adli[currentpos++];
}

void reg_adler_finished() {
  CHECK(state == ADLER_OUTPUTTING);
  state = ADLER_IDLE;
}

