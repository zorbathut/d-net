
#include "stream.h"

#include "debug.h"

// We want to use the fewest reads possible on average. This means reading the largest chunks possible.
void IStream::read(char *storage, int size) {
  CHECK(size >= 0);
  int trfb = min(size, buffend - buffpos);
  memcpy(storage, buff + buffpos, trfb);
  buffpos += trfb;
  storage += trfb;
  size -= trfb; // Read everything we can out of the existing buffer
  
  if(size) { // We need more!
    if(size >= sizeof(buff)) {
      CHECK(read_worker(storage, size) == size); // Just pull it straight into the buffer, it's easier this way.
    } else {
      buffend = read_worker(buff, sizeof(buff));
      CHECK(buffend >= size);
      memcpy(storage, buff, size);
      buffpos = size;
    }
  }
}
  
IStream::IStream() : buffpos(sizeof(buff)), buffend(sizeof(buff)) { };

void OStream::write(const char *storage, int size) {
  CHECK(size >= 0);
  // If it fits within the buffer, put it there.
  // If it doesn't fit within the buffer, but does fit within the next buffer, fill the buffer then put it in the next one.
  // If it doesn't fit within this buffer or the next buffer, clear the buffer and then write it directly.
  if(buffpos + size < sizeof(buff)) {
    memcpy(buff + buffpos, storage, size);
    buffpos += size;
  } else if(buffpos + size < sizeof(buff) * 2) {
    memcpy(buff + buffpos, storage, sizeof(buff) - buffpos);
    write_worker(buff, sizeof(buff));
    storage += sizeof(buff) - buffpos;
    size -= sizeof(buff) - buffpos;
    memcpy(buff, storage, size);
    buffpos = size;
  } else {
    write_worker(buff, buffpos);
    write_worker(storage, size);
    buffpos = 0;
  }
}

void OStream::flush() {
  write_worker(buff, buffpos);
  buffpos = 0;
}

OStream::OStream() : buffpos(0) { };
OStream::~OStream() { CHECK(buffpos == 0); };
