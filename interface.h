#ifndef DNET_INTERFACE
#define DNET_INTERFACE

#include <vector>

using namespace std;

#include "input.h"
#include "ai.h"

void interfaceInit();
bool interfaceRunTick( const vector< Controller > &control );
void interfaceRunAi(const vector<Ai *> &ais);
void interfaceRenderToScreen();

#endif
