#ifndef DNET_ADLER32_UTIL
#define DNET_ADLER32_UTIL

#include "adler32.h"

#include <map>
#include <set>
#include <vector>
#include <string>
#include <utility>

using namespace std;

template<typename T, typename U> void adler(Adler32 *adl, const pair<T, U> &tee) {
  adler(adl, tee.first);
  adler(adl, tee.second);
}

template<typename T> void adler(Adler32 *adl, const vector<T> &tee) {
  adler(adl, tee.size());
  for(int i = 0; i < tee.size(); i++)
    adler(adl, tee[i]);
}

template<typename T, typename U> void adler(Adler32 *adl, const map<T, U> &tee) {
  adler(adl, tee.size());
  for(typename map<T, U>::const_iterator itr = tee.begin(); itr != tee.end(); itr++)
    adler(adl, *itr);
}

template<typename T> void adler(Adler32 *adl, const set<T> &tee) {
  adler(adl, tee.size());
  for(typename set<T>::const_iterator itr = tee.begin(); itr != tee.end(); itr++)
    adler(adl, *itr);
}

template<typename T, int K> void adler(Adler32 *adl, const T (&tee)[K]) {
  for(int i = 0; i < K; i++)
    adler(adl, tee[i]);
}

inline void adler(Adler32 *adl, const string &str) {
  adler(adl, str.size());
  for(int i = 0; i < str.size(); i++)
    adl->addByte(str[i]);
}

#endif
