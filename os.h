#ifndef DNET_OS
#define DNET_OS

#include <string>

using namespace std;

// Debug stuff
void outputDebugString(const string &str);

void dumpStackTrace();

bool isUnoptimized();

// This can be used by stack traces and such
void set_exename(const string &str); // this should be removed eventually I think

// OS stuff
void seriouslyCrash() __attribute__((__noreturn__)); // apparently this is needed. Why? Because SDL is stupid.

string getConfigDirectory(); // oy
void makeConfigDirectory();

string getTempFilename();

pair<int, int> getScreenRes();

void SpawnProcess(const string &exec);

#endif
