#ifndef DNET_LEVEL
#define DNET_LEVEL

#include "coord.h"

#include <vector>
#include <set>
#include <map>

using namespace std;

class Level {
public:
    vector<vector<Coord2> > paths;

    set<int> playersValid;

    map<int, vector<pair<Coord2, float> > > playerStarts;

    void makeProperSolids();

};

Level loadLevel(const string &str);

#endif
