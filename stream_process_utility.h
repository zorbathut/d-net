#ifndef DNET_STREAM_PROCESS_UTILITY
#define DNET_STREAM_PROCESS_UTILITY

#include "stream.h"

#include <utility>

using namespace std;

template<typename T, typename U> struct IStreamReader<pair<T, U> > { static void read(IStream *istr, pair<T, U> *storage) {
  istr->read(&storage->first);
  istr->read(&storage->second);
} };

template<typename T, typename U> struct OStreamWriter<pair<T, U> > { static void write(OStream *ostr, const pair<T, U> &storage) {
  ostr->write(storage.first);
  ostr->write(storage.second);
} };

#endif
