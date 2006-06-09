#ifndef DNET_INTERFACE
#define DNET_INTERFACE

#include "input.h"

using namespace std;

class Ai;

void interfaceInit();
bool interfaceRunTick( const vector< Controller > &control );
void interfaceRunAi(const vector<Ai *> &ais);
void interfaceRenderToScreen();

#endif
