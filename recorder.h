#ifndef DNET_RECORDER
#define DNET_RECORDER

#include "itemdb.h"

#include <cstdio>

using namespace std;

class Recorder {
public:
  void warhead(const IDBWarhead *warhead, int tank_id, vector<pair<float, int> > &adjacencies);
  void metastats(int cycles, const vector<pair<int, int> > &pertank);

  Recorder(FILE *output);
  ~Recorder();

private:
  map<string, int> lines;

  FILE *output;
};

#endif
