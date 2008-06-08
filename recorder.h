#ifndef DNET_RECORDER
#define DNET_RECORDER

#include "itemdb.h"

using namespace std;

bool operator<(const FileShopcache::Entry &lhs, const FileShopcache::Entry &rhs);

class Recorder {
public:
  void warhead(const IDBWarhead *warhead, float factor, int tank_id, vector<pair<float, int> > &adjacencies);
  void metastats(int cycles, const vector<int> &damageframes);

  bool hasItems() const;

  FileShopcache data() const;

private:
  map<FileShopcache::Entry, int> lines;
  int cycles;
  vector<int> damageframes;
};

#endif
