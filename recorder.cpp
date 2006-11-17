
#include "recorder.h"

void Recorder::movement(const vector<pair<bool, pair<Coord2, float> > > &tankpos) {
  lastmovement = "";
  for(int i = 0; i < tankpos.size(); i++) {
    string ps;
    if(tankpos[i].first) {
      ps = StringPrintf("%s %s %s", tankpos[i].second.first.x.rawstr().c_str(), tankpos[i].second.first.y.rawstr().c_str(), rawstrFromFloat(tankpos[i].second.second).c_str());
    } else {
      ps = "(dead)";
    }
    lastmovement += StringPrintf("  cmd=position %d %s\n", i, ps.c_str());
  }
  
  if(lastmovement == curposition)
    lastmovement.clear();
}

void Recorder::warhead(const IDBWarheadAdjust &warhead, Coord2 pos, int tank_id) {
  bool getsRecorded = false;
  if(warhead.radiusfalloff() > 0)
    getsRecorded = true;
  
  if(warhead.impactdamage() > 0 && tank_id != -1)
    getsRecorded = true;
  
  if(!getsRecorded)
    return;
  
  if(lastmovement.size()) {
    fprintf(output, "%s", lastmovement.c_str());
    curposition = lastmovement;
    lastmovement.clear();
  }
  
  string name = nameFromWarhead(warhead.base());
  string tank;
  if(tank_id == -1)
    tank = "none";
  else
    tank = StringPrintf("%d", tank_id);
  fprintf(output, "  cmd=warhead %s %s %s %s\n", name.c_str(), pos.x.rawstr().c_str(), pos.y.rawstr().c_str(), tank.c_str());
}

Recorder::Recorder(FILE *output) : output(output) { };
