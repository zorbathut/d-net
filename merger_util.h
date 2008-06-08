#ifndef MERGER_UTIL
#define MERGER_UTIL

#include "parse.h"

#include <set>
using namespace std;



vector<string> parseCsv(const string &in);
string splice(const string &source, const string &splicetext);
string splice(const string &source, float splicevalue);
string suffix(const string &name, int position = 1);

void checkForExtraMerges(const kvData &kvd);

string findName(const string &thistoken, const set<string> &table);

template<typename T> string findName(const string &thistoken, const map<string, T> &table) {
  set<string> temp;
  for(typename map<string, T>::const_iterator itr = table.begin(); itr != table.end(); itr++)
    temp.insert(itr->first);
  return findName(thistoken, temp);
}

class BaseNamer {
public:
  string getName(const vector<string> &line) {
    return line[0];
  }
};

#endif
