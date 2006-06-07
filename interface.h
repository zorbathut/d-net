#ifndef DNET_INTERFACE
#define DNET_INTERFACE

#include <vector>

using namespace std;

#include "input.h"

class Ai;

void interfaceInit();
bool interfaceRunTick( const vector< Controller > &control );
void interfaceRunAi(const vector<Ai *> &ais);
void interfaceRenderToScreen();

#endif
