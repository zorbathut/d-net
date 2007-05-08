#ifndef MERGER_UTIL
#define MERGER_UTIL

#include <vector>
#include <string>
#include <map>

#include "debug.h"

using namespace std;

vector<string> parseCsv(const string &in);
string splice(const string &source, const string &splicetext);
string splice(const string &source, float splicevalue);
string suffix(const string &name, int position = 1);

template<typename T> string findName(const string &thistoken, const map<string, T> &table) {
  string tts;
  for(int i = 0; i < thistoken.size(); i++)
    if(isalpha(thistoken[i]) || thistoken[i] == '.')
      tts += tolower(thistoken[i]);
  CHECK(tts.size());
  
  string ct;
  for(typename map<string, T>::const_iterator itr = table.begin(); itr != table.end(); itr++) {
    CHECK(itr->first.size());
    string tx;
    for(int i = 0; i < itr->first.size(); i++)
      if(isalpha(itr->first[i]) || thistoken[i] == '.')
        tx += tolower(itr->first[i]);
    //dprintf("Comparing %s and %s\n", tx.c_str(), tts.c_str());
    if(tx == tts) {
      CHECK(!ct.size());
      ct = itr->first;
    }
  }
  return ct;
}

#endif
