#ifndef DNET_LEVEL
#define DNET_LEVEL

#include "dvec2.h"

#include <vector>
#include <set>
#include <map>

using namespace std;

class Level {
public:
    vector<vector<Float2> > paths;

    set<int> playersValid;

    map<int, vector<pair<Float2, float> > > playerStarts;
};

Level loadLevel(const string &str);

#endif
