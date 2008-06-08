#ifndef DNET_STREAM_GZ
#define DNET_STREAM_GZ

#include "stream.h"

using namespace std;

class IStreamGz : public IStream {
private:
  void *gz;
  
  int read_worker(char *buff, int avail);
public:

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

  OStreamGz(const string &fname); // THIS IS NOT CURRENTLY THREADSAFE
  ~OStreamGz();
};

#endif
