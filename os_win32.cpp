
#include "os.h"

#include <string>

#include <windows.h>

using namespace std;

void outputDebugString(const string &str) {
  OutputDebugString(str.c_str());
}

pair<int, int> getCurrentScreenSize() {
  return make_pair(GetSystemMetrics(SM_CXSCREEN), GetSystemMetrics(SM_CYSCREEN));
}
