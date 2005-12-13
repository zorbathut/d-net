
#include "os.h"

#include <windows.h>

void outputDebugString(const string &str) {
  OutputDebugString(str.c_str());
}

pair<int, int> getCurrentScreenSize() {
  return make_pair(GetSystemMetrics(SM_CXSCREEN), GetSystemMetrics(SM_CYSCREEN));
}
