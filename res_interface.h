#ifndef DNET_RESINTERFACE
#define DNET_RESINTERFACE






#include <vector>

using namespace std;

bool setResolution(pair<int, int> res, float aspect, bool fullscreen);

vector<pair<int, int> > getResolutions();
pair<int, int> getCurrentResolution();
float getCurrentAspect();
bool getCurrentFullscreen();

#endif
