#ifndef DNET_STREAM_GZ
#define DNET_STREAM_GZ

#include "stream.h"

using namespace std;

class IStreamGz : public IStream {
private:
  void *gz;
  
  int read_worker(char *buff, int avail);
public:

  operator const void*() const;

  IStreamGz(const string &fname);
  ~IStreamGz();
};

class OStreamGz : public OStream {
private:
  void *gz;
  
  void write_worker(const char *buff, int avail);

  friend void flushOstreamGzOnCrash();
  void flush_and_terminate();
public:

  operator const void*() const;

  OStreamGz(const string &fname, int level = 5); // THIS IS NOT CURRENTLY THREADSAFE
  ~OStreamGz();
};

#endif
