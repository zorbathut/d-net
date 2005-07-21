#ifndef DNET_INTERFACE
#define DNET_INTERFACE

#include <vector>

using namespace std;

#include "util.h"

void interfaceInit();
bool interfaceRunTick( const vector< Controller > &control, const Keystates &keyb );
void interfaceRenderToScreen();

#endif
