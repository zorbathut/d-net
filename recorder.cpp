
#include "recorder.h"

void Recorder::warhead(const IDBWarhead *warhead, int tank_id, vector<pair<float, int> > &adjacencies) {
  CHECK(tank_id != -1 || adjacencies.size());
  
  string tank;
  if(tank_id == -1)
    tank = "none";
  else
    tank = StringPrintf("%d", tank_id);
  
  string line;
  
  line = StringPrintf("%s %s", nameFromIDB(warhead).c_str(), tank.c_str());
  
  for(int i = 0; i < adjacencies.size(); i++)
    line += StringPrintf(" %s %d", rawstrFromFloat(adjacencies[i].first).c_str(), adjacencies[i].second);
  
  lines[line]++;
}

Recorder::Recorder(FILE *output) : output(output) { };
Recorder::~Recorder() {
  for(map<string, int>::const_iterator itr = lines.begin(); itr != lines.end(); itr++) {
    fprintf(output, "  x=%d %s\n", itr->second, itr->first.c_str());
  }
};