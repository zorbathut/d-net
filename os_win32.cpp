
#include "os.h"

#include "debug.h"
#include "util.h"

#include <string>
#include <fstream>
#include <vector>
#include <unistd.h>
#include <windows.h>

using namespace std;

void outputDebugString(const string &str) {
  OutputDebugString(str.c_str());
}

pair<int, int> getCurrentScreenSize() {
  return make_pair(GetSystemMetrics(SM_CXSCREEN), GetSystemMetrics(SM_CYSCREEN));
}

#if defined(__GNUG__)
 
const unsigned int kStackTraceMax = 20;
const unsigned int kMaxClassDepth = kStackTraceMax * 2 + 1;
//BOOST_STATIC_ASSERT(kMaxClassDepth % 2 == 1); // Must be odd.
 
template <unsigned int S, unsigned int N = 1> struct StackTracer {
  static void printStack(vector<const void *> *vek) {
    if (!__builtin_frame_address(N))
      return;
 
    if (const void * const p = __builtin_return_address(N)) {
      vek->push_back(p);
      // Because this is recursive(ish), we may have to go down the stack by 2.
      StackTracer<S, N + S>::printStack(vek);
    }
  }
};
 
template <> struct StackTracer<1, kStackTraceMax> {
  static void printStack(vector<const void *> *vek) {}
};

template <> struct StackTracer<2, kMaxClassDepth> {
  static void printStack(vector<const void *> *vek) {}
};

inline bool verifyInlined(const void *const p) {
  return __builtin_return_address(1) == p;
}

bool testInlined() {
  return verifyInlined(__builtin_return_address(1));
}
 
void dumpStackTrace() {
  dprintf("Stacktracing\n");
  vector<const void *> stack;
  if(testInlined()) {
    StackTracer<1>::printStack(&stack);
  } else {
    StackTracer<2>::printStack(&stack);
  }

  string line = "addr2line -e d-net.exe ";
  for(int i = 0; i < stack.size(); i++)
    line += StringPrintf("%p ", stack[i]);
  line += "> addr2linetmp.txt";
  int rv = system(line.c_str());
  if(!rv) {
    {
      ifstream ifs("addr2linetmp.txt");
      string lin;
      while(getline(ifs, lin))
        dprintf("  %s", lin.c_str());
    }
    unlink("addr2linetmp.txt");
  } else {
    dprintf("Couldn't call addr2line\n");
  }
  dprintf("\n");
}

#endif
