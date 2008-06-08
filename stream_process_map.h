#ifndef DNET_STREAM_PROCESS_MAP
#define DNET_STREAM_PROCESS_MAP

#include "stream.h"

#include <map>
using namespace std;



template<typename T, typename U> struct IStreamReader<map<T, U> > { static bool read(IStream *istr, map<T, U> *storage) {
  int count;
  storage->clear();
  if(istr->tryRead(&count)) return true;
  for(int i = 0; i < count; i++) {
    pair<T, U> v;
    if(istr->tryRead(&v))
      return true;
    CHECK(!storage->count(v.first));
    storage->insert(v);
  }
  return false;
} };

template<typename T, typename U> struct OStreamWriter<map<T, U> > { static void write(OStream *ostr, const map<T, U> &storage) {
  ostr->write(storage.size());
  for(typename map<T, U>::const_iterator itr = storage.begin(); itr != storage.end(); itr++)
    ostr->write(*itr);
} };

#endif
