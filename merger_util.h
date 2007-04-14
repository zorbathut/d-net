#ifndef MERGER_UTIL
#define MERGER_UTIL

#include <vector>
#include <string>
#include <map>

using namespace std;

vector<string> parseCsv(const string &in);
string splice(const string &source, const string &splicetext);
string suffix(const string &name);

template<typename T> string findName(const string &thistoken, const map<string, T> &table) {
  string tts;
  for(int i = 0; i < thistoken.size(); i++)
    if(thistoken[i] != ' ')
      tts += thistoken[i];
  CHECK(tts.size());
  
  string ct;
  for(typename map<string, T>::const_iterator itr = table.begin(); itr != table.end(); itr++) {
    CHECK(itr->first.size());
    string tx;
    for(int i = 0; i < itr->first.size(); i++)
      if(itr->first[i] != ' ')
        tx += itr->first[i];
    if(tx == tts) {
      CHECK(!ct.size());
      ct = itr->first;
    }
  }
  return ct;
}

#endif
