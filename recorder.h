#ifndef DNET_RECORDER
#define DNET_RECORDER

#include "itemdb.h"

#include <cstdio>

using namespace std;

class Recorder {
public:
  
  void movement(const vector<pair<bool, Coord2> > &tankpos);
  void warhead(const IDBWarheadAdjust &warhead, Coord2 pos, int tank_id);

  Recorder(FILE *output);

private:
  FILE *output;
};

#endif
