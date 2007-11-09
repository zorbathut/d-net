#ifndef DNET_STREAM_PROCESS_VECTOR
#define DNET_STREAM_PROCESS_VECTOR

#include "stream_process_primitive.h"

#include <vector>

using namespace std;

template<typename T> struct IStreamReader<vector<T> > { static void read(IStream *istr, vector<T> *storage) {
  int count;
  istr->read(&count);
  storage->resize(count);
  for(int i = 0; i < count; i++)
    istr->read(&(*storage)[i]);
} };

template<typename T> struct OStreamWriter<vector<T> > { static void write(OStream *ostr, const vector<T> &storage) {
  ostr->write(storage.size());
  for(int i = 0; i < storage.size(); i++)
    ostr->write(storage[i]);
} };

#endif
