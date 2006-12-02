
#include "debug.h"

#include "os.h"

#include <stdarg.h>
#include <assert.h>

#include <vector>

using namespace std;

int frameNumber = -1;
void *stackStart;

static vector<StackPrinter*> dbgstack;

StackPrinter::StackPrinter() {
  dbgstack.push_back(this);
}
StackPrinter::~StackPrinter() {
  CHECK(dbgstack.back() == this);
  dbgstack.pop_back();
}

void StackString::Print() const {
  dprintf("  %s", str_.c_str());
}
StackString::StackString(const string &str) : str_(str) {
  //dprintf("%s\n", str.c_str());
};

void PrintDebugStack() {
  for(int i = (int)dbgstack.size() - 1; i >= 0; i--) {
    dprintf("Stack entry:\n");
    dbgstack[i]->Print();
  }
  dprintf("End of stack\n");
  dumpStackTrace();
}

void (*crashfunc)() = NULL;

void registerCrashFunction(void (*fct)()) {
  CHECK(!crashfunc);
  crashfunc = fct;
}
void unregisterCrashFunction(void (*fct)()) {
  CHECK(crashfunc == fct);
  crashfunc = NULL;
}

#ifdef VECTOR_PARANOIA
class VectorParanoiaChecker {
  static void throwshit() {
    throw int();
  }
public:
  VectorParanoiaChecker() {
    vector<int> test(100);
    registerCrashFunction(&throwshit);
    try {
      test[100];
      unregisterCrashFunction(&throwshit);
      dprintf("VECTOR PARANOIA FAILED");
      CHECK(0);
    } catch (int x) {
      unregisterCrashFunction(&throwshit);
    }
  }
} paranoia;
#endif

void CrashHandler(const char *fname, int line) {
  if(crashfunc)
    (*crashfunc)();
};

void crash() {
  dprintf("About to crash!\n");
  seriouslyCrash();
  dprintf("Crashed!\n");
}

int dprintf(const char *bort, ...) {

  // this is duplicated code with StringPrintf - I should figure out a way of combining these
  static vector< char > buf(2);
  va_list args;
  buf[buf.size() - 1] = 1;

  int done = 0;
  bool noresize = false;
  do {
    if(done && !noresize)
      buf.resize(buf.size() * 2);
    va_start(args, bort);
    done = vsnprintf(&(buf[0]), buf.size() - 1,  bort, args);
    if(done >= (int)buf.size()) {
      assert(noresize == false);
      assert(buf[buf.size() - 2] == 0);
      buf.resize(done + 2);
      done = -1;
      noresize = true;
    } else {
      assert(done < (int)buf.size());
    }
    va_end(args);
  } while(done == buf.size() - 1 || done == -1);

  assert(done < (int)buf.size());

  outputDebugString(&(buf[0]));

  return 0;

};
