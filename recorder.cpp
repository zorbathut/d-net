
#include "recorder.h"

void Recorder::warhead(const IDBWarheadAdjust &warhead, Coord2 pos, int tank_id) {
  bool getsRecorded = false;
  if(warhead.radiusfalloff() > 0)
    getsRecorded = true;
  
  if(warhead.impactdamage() > 0 && tank_id != -1)
    getsRecorded = true;
  
  if(!getsRecorded)
    return;
  
  string name = nameFromWarhead(warhead.base());
  string tank;
  if(tank_id == -1)
    tank = "none";
  else
    tank = StringPrintf("%d", tank_id);
  fprintf(output, "  cmd=warhead %s %s %s %s\n", name.c_str(), pos.x.rawstr().c_str(), pos.y.rawstr().c_str(), tank.c_str());
}

Recorder::Recorder(FILE *output) : output(output) { };
