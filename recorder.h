#ifndef DNET_RECORDER
#define DNET_RECORDER

#include "itemdb.h"

#include <cstdio>

using namespace std;

class Recorder {
public:
  
  void movement(const vector<pair<bool, pair<Coord2, float> > > &tankpos);
  void warhead(const IDBWarheadAdjust &warhead, Coord2 pos, int tank_id);

  Recorder(FILE *output);

private:
  string lastmovement;
  string curposition;

  FILE *output;
};

#endif
