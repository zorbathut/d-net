#ifndef DNET_RESINTERFACE
#define DNET_RESINTERFACE

#include <vector>
#include <utility>

using namespace std;

bool setResolution(int x, int y, float aspect, bool fullscreen);

vector<pair<int, int> > getResolutions();

#endif
