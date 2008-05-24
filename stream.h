#ifndef DNET_STREAM
#define DNET_STREAM

#include "debug.h"

#include <boost/noncopyable.hpp>

template<typename T> struct IStreamReader;
template<typename T> struct OStreamWriter;

class IStream : boost::noncopyable {
private:
  char buff[16384];
  int buffpos;
  int buffend;
  
  virtual int read_worker(char *buff, int avail) = 0;
public:
  
  void read(char *storage, int size);
  template<typename T> void read(T *storage) { CHECK(!IStreamReader<T>::read(this, storage)); }
  
  // non-zero is error. may leave storage in unknown state on failure.
  bool tryRead(char *storage, int size);
  template<typename T> bool tryRead(T *storage) { return IStreamReader<T>::read(this, storage); }
  
  int readInt();
  
  IStream();
  virtual ~IStream() { };
};

class OStream : boost::noncopyable {
private:
  char buff[16384];
  int buffpos;
  
  virtual void write_worker(const char *buff, int avail) = 0;
public:
  
  void write(const char *storage, int size);
  template<typename T> void write(const T &storage) { OStreamWriter<T>::write(this, storage); }
  
  void flush();

  OStream();
  virtual ~OStream();
};

template<typename T> bool stream_write(OStream *ostr, const T &storage) { return storage.stream_tryWrite(ostr); }
template<typename T> struct OStreamWriter { static void write(OStream *ostr, const T &storage) { stream_write(ostr, storage); } };

template<typename T> bool stream_read(IStream *istr, T *storage) { return storage->stream_read(istr); }
template<typename T> struct IStreamReader { static bool read(IStream *istr, T *storage) { return stream_read(istr, storage); } };

#endif
