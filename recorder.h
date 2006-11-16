#ifndef DNET_RECORDER
#define DNET_RECORDER

#include "itemdb.h"

class Recorder {
public:
  void warhead(const IDBWarheadAdjust &warhead, Coord2 pos, int tank_id);
};

#endif
