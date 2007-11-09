#ifndef DNET_STREAM_PROCESS_STRING
#define DNET_STREAM_PROCESS_STRING

#include "stream_process_primitive.h"

#include <string>

using namespace std;

template<> struct IStreamReader<string> { static void read(IStream *istr, string *storage) {
  int count;
  istr->read(&count);
  vector<char> dt(count);
  istr->read(&dt[0], count);
  storage->resize(count);
  copy(dt.begin(), dt.end(), storage->begin());
} };

template<> struct OStreamWriter<string> { static void write(OStream *ostr, const string &storage) {
  ostr->write(storage.size());
  ostr->write(storage.data(), storage.size());
} };

#endif
