
#include "recorder.h"


using namespace std;

bool operator<(const FileShopcache::Entry &lhs, const FileShopcache::Entry &rhs) {
  if(lhs.warhead != rhs.warhead) return lhs.warhead < rhs.warhead;
  if(lhs.count != rhs.count) return lhs.count < rhs.count;
  if(lhs.mult != rhs.mult) return lhs.mult < rhs.mult;
  if(lhs.impact != rhs.impact) return lhs.impact < rhs.impact;
  if(lhs.adjacencies != rhs.adjacencies) return lhs.adjacencies < rhs.adjacencies;
  return false;
}

void Recorder::warhead(const IDBWarhead *warhead, float factor, int tank_id, vector<pair<float, int> > &adjacencies) {
  CHECK(tank_id != -1 || adjacencies.size());
  CHECK(tank_id >= 0 && tank_id < 16 || tank_id == -1);
  
  {
    bool doesdamage = false;
    for(int i = 0; i < IDBAdjustment::DAMAGE_LAST; i++)
      if(warhead->impactdamage[i] != 0 || warhead->radiusdamage[i] != 0)
        doesdamage = true;
      
    if(!doesdamage)
      return;
  }
  
  lines[FileShopcache::Entry(nameFromIDB(warhead), factor, tank_id, adjacencies)]++;
}

void Recorder::metastats(int in_cycles, const vector<int> &in_damageframes) {
  cycles = in_cycles;
  damageframes = in_damageframes;
}

bool Recorder::hasItems() const {
  return lines.size();
}

FileShopcache Recorder::data() const {
  FileShopcache rv;
  for(map<FileShopcache::Entry, int>::const_iterator itr = lines.begin(); itr != lines.end(); itr++) {
    FileShopcache::Entry fsci = itr->first;
    fsci.count = itr->second;
    rv.entries.push_back(fsci);
  }
  rv.cycles = cycles;
  rv.damageframes = damageframes;
  return rv;
}
