#ifndef DNET_SMARTPTR_UTIL
#define DNET_SMARTPTR_UTIL

#include "smartptr.h"

#include <vector>
using namespace std;



template <typename T> vector<const T*> ptrize(const vector<smart_ptr<T> > &vt) {
  vector<const T*> rv;
  for(int i = 0; i < vt.size(); i++)
    rv.push_back(vt[i].get());
  return rv;
}

template <typename T> vector<T*> ptrize(vector<smart_ptr<T> > &vt) {
  vector<T*> rv;
  for(int i = 0; i < vt.size(); i++)
    rv.push_back(vt[i].get());
  return rv;
}

#endif
