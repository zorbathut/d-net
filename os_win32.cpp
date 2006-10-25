
#include "os.h"

#include "debug.h"
#include "util.h"

#include <fstream>
#include <vector>
#include <signal.h>
#include <unistd.h>
#include <windows.h>

using namespace std;

typedef void (*sighandler_t)(int);

class SignalHandler {
public:
  static void signal_h(int signum) {
    CHECK(0);
  }
  
  SignalHandler() {
    signal(SIGSEGV, &signal_h);
  }
} sighandler;

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
  vector<const void *> stack;
  if(testInlined()) {
    dprintf("Stacktracing (inlined)\n");
    StackTracer<1>::printStack(&stack);
  } else {
    dprintf("Stacktracing\n");
    StackTracer<2>::printStack(&stack);
  }

  vector<pair<string, string> > tokens;
  {
    string line = "addr2line -f -e d-net.exe ";
    for(int i = 0; i < stack.size(); i++)
      line += StringPrintf("%p ", stack[i]);
    line += "> addr2linetmp.txt";
    int rv = system(line.c_str());
    if(!rv) {
      {
        ifstream ifs("addr2linetmp.txt");
        string lin;
        while(getline(ifs, lin)) {
          string tlin;
          getline(ifs, tlin);
          tokens.push_back(make_pair(lin, tlin));
        }
      }
      unlink("addr2linetmp.txt");
    } else {
      dprintf("Couldn't call addr2line\n");
      return;
    }
  }
  
  {
    string line = "c++filt -n -s gnu-v3 ";
    for(int i = 0; i < tokens.size(); i++)
      line += tokens[i].first + " ";
    line += "> cppfilttmp.txt";
    int rv = system(line.c_str());
    if(!rv) {
      {
        ifstream ifs("cppfilttmp.txt");
        string lin;
        int ct = 0;
        while(getline(ifs, lin)) {
          if(lin.size() && lin[0] == '_')
            lin.erase(lin.begin());
          dprintf("  %s - %s", tokens[ct].second.c_str(), lin.c_str());
          ct++;
        }
      }
      unlink("cppfilttmp.txt");
    } else {
      dprintf("Couldn't call c++filt\n");
      return;
    }
  }
  dprintf("\n");
}

bool isUnoptimized() {
  return !testInlined();
}

#endif

void seriouslyCrash() {
  ExitProcess(1);
}
