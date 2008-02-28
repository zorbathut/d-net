
#include "audit.h"

#include "debug.h"

#include <vector>

using namespace std;

static vector<const AuditIgnore *> aigl;

AuditIgnore::AuditIgnore() {
  aigl.push_back(this);
}
AuditIgnore::~AuditIgnore() {
  CHECK(aigl.size());
  CHECK(aigl.back() == this);
  aigl.pop_back();
}

enum AuditState { AUDIT_IDLE, AUDIT_COMPARE_PRIMING, AUDIT_COMPARING, AUDIT_CREATING, AUDIT_COMPARING_PAUSED, AUDIT_CREATING_PAUSED, AUDIT_OUTPUTTING } state;

static int currentpos = 0;
static vector<unsigned long> adli;

void audit_start_create() {
  CHECK(state == AUDIT_IDLE);
  adli.clear();
  state = AUDIT_CREATING;
}
void audit_start_compare() {
  CHECK(state == AUDIT_IDLE);
  adli.clear();
  state = AUDIT_COMPARE_PRIMING;
}

void audit_start_compare_add(unsigned long unl) {
  CHECK(state == AUDIT_COMPARE_PRIMING);
  adli.push_back(unl);
}
void audit_start_compare_done() {
  CHECK(state == AUDIT_COMPARE_PRIMING);
  state = AUDIT_COMPARING;
  currentpos = 0;
}

void audit_worker(const Adler32 &adl, const char *file, int line) {
  audit_worker(adl.output(), file, line);
}

void audit_worker(unsigned long dat, const char *file, int line) {
  //dprintf("%s, %d\n", file, line);
  if(aigl.size()) {
  } else if(state == AUDIT_COMPARING) {
    if(currentpos && adli[currentpos - 1] == dat)
      return;
    if(currentpos >= adli.size()) {
      dprintf("Too many adler items at %s:%d frame %d (%d valid)\n", file, line, frameNumber, adli.size());
      CHECK(0);
    }
    if(adli[currentpos] != dat) {
      dprintf("Audit consistency failure at %s:%d %d (%08lx vs %08lx)\n", file, line, frameNumber, adli[currentpos], dat);
      CHECK(0);
    }
    currentpos++;
  } else if(state == AUDIT_CREATING) {
    if(!adli.size() || adli.back() != dat)
      adli.push_back(dat);
  } else {
    CHECK(0);
  }
}

void audit_pause() {
  if(state == AUDIT_COMPARING) {
    state = AUDIT_COMPARING_PAUSED;
  } else if(state == AUDIT_CREATING) {
    state = AUDIT_CREATING_PAUSED;
  } else {
    CHECK(0);
  }
}

void audit_unpause() {
  if(state == AUDIT_COMPARING_PAUSED) {
    state = AUDIT_COMPARING;
  } else if(state == AUDIT_CREATING_PAUSED) {
    state = AUDIT_CREATING;
  } else {
    CHECK(0);
  }
}

void audit_register_finished() {
  if(state == AUDIT_COMPARING) {
    if(currentpos != adli.size()) {
      dprintf("%d, %d\n", currentpos, adli.size());
      CHECK(currentpos == adli.size());
    }
  } else if(state == AUDIT_CREATING) {
  } else {
    CHECK(0);
  }
  state = AUDIT_OUTPUTTING;
  currentpos = 0;
}

int audit_read_count() {
  CHECK(state == AUDIT_OUTPUTTING || state == AUDIT_IDLE);
  return adli.size();
}
unsigned long audit_read_ref() {
  CHECK(state == AUDIT_OUTPUTTING);
  CHECK(currentpos <= adli.size());
  return adli[currentpos++];
}

void audit_finished() {
  CHECK(state == AUDIT_OUTPUTTING);
  state = AUDIT_IDLE;
}

