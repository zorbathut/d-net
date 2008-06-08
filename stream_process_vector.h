#ifndef DNET_STREAM_PROCESS_VECTOR
#define DNET_STREAM_PROCESS_VECTOR

#include "stream.h"

#include <vector>

using namespace std;

template<typename T> struct IStreamReader<vector<T> > { static bool read(IStream *istr, vector<T> *storage) {
  int count;
  if(istr->tryRead(&count)) return true;
  storage->resize(count);
  for(int i = 0; i < count; i++)
    if(istr->tryRead(&(*storage)[i]))
      return true;
  return false;
} };

template<typename T> struct OStreamWriter<vector<T> > { static void write(OStream *ostr, const vector<T> &storage) {
  ostr->write(storage.size());
  for(int i = 0; i < storage.size(); i++)
    ostr->write(storage[i]);
} };

#endif
