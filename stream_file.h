#ifndef DNET_STREAM_FILE
#define DNET_STREAM_FILE

#include "stream.h"

using namespace std;

class IStreamFile : public IStream {
private:
  FILE *file;
  
  int read_worker(char *buff, int avail);
public:

  operator void*() const;

  IStreamFile(const string &fname);
  ~IStreamFile();
};

class OStreamFile : public OStream {
private:
  FILE *file;
  
  void write_worker(const char *buff, int avail);
public:

  operator void*() const;

  OStreamFile(const string &fname);
  ~OStreamFile();
};

#endif
