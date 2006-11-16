
#include "recorder.h"

void Recorder::warhead(const IDBWarheadAdjust &warhead, Coord2 pos, int tank_id) {
  string name = nameFromWarhead(warhead.base());
  dprintf("%s, %f,%f, %d\n", name.c_str(), pos.x.toFloat(), pos.y.toFloat(), tank_id);
}
