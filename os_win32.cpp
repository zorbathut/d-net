
#include "os.h"

#include "debug.h"

#include <string>

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
  static void printStack() {
    if (!__builtin_frame_address(N))
      return;
 
    if (const void * const p = __builtin_return_address(N)) {
      dprintf("  %p", p);
      // Because this is recursive(ish), we have to go down the stack by 2.
      StackTracer<S, N + S>::printStack();
    }
  }
};
 
template <> struct StackTracer<1, kStackTraceMax> {
  static void printStack() {}
};

template <> struct StackTracer<2, kMaxClassDepth> {
  static void printStack() {}
};

inline bool verifyInlined(const void *const p) {
  return __builtin_return_address(1) == p;
}

bool testInlined() {
  return verifyInlined(__builtin_return_address(1));
}
 
void dumpStackTrace() {
  dprintf("*** Stack Trace Follows ***\n\n");
  if(testInlined()) {
    StackTracer<1>::printStack();
  } else {
    StackTracer<2>::printStack();
  }
}
 
#endif
