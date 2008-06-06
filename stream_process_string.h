#ifndef DNET_STREAM_PROCESS_STRING
#define DNET_STREAM_PROCESS_STRING

#include "stream_process_primitive.h"

#include <string>
#include <vector>

using namespace std;

template<> struct IStreamReader<string> { static bool read(IStream *istr, string *storage) {
  int count;
  if(istr->tryRead(&count)) return true;
  if(!count) {
    storage->clear();
    return false;
  }
  vector<char> dt(count);
  if(istr->tryRead(&dt[0], count)) return true;
  storage->resize(count);
  copy(dt.begin(), dt.end(), storage->begin());
  return false;
} };

template<> struct OStreamWriter<string> { static void write(OStream *ostr, const string &storage) {
  ostr->write(storage.size());
  ostr->write(storage.data(), storage.size());
} };

#endif
