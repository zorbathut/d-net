#ifndef DNET_OS
#define DNET_OS

#include <string>

using namespace std;

void outputDebugString(const string &str);

pair<int, int> getCurrentScreenSize();

void dumpStackTrace();

bool isUnoptimized();

void seriouslyCrash() __attribute__((__noreturn__)); // apparently this is needed. Why? Because SDL is stupid.

#endif
