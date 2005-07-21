#ifndef DNET_INTERFACE
#define DNET_INTERFACE

#include <vector>

using namespace std;

#include "util.h"

void interfaceInit();
bool interfaceRunTick( const vector< Keystates > &keys );
void interfaceRenderToScreen();

#endif
