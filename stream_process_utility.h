#ifndef DNET_STREAM_PROCESS_UTILITY
#define DNET_STREAM_PROCESS_UTILITY

#include "stream.h"

#include <utility>

using namespace std;

template<typename T, typename U> struct IStreamReader<pair<T, U> > { static bool read(IStream *istr, pair<T, U> *storage) {
  if(istr->tryRead(&storage->first)) return true;
  if(istr->tryRead(&storage->second)) return true;
  return false;
} };

template<typename T, typename U> struct OStreamWriter<pair<T, U> > { static void write(OStream *ostr, const pair<T, U> &storage) {
  ostr->write(storage.first);
  ostr->write(storage.second);
} };

#endif
