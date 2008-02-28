
#include "stream_gz.h"

#include <zlib.h>

int IStreamGz::read_worker(char *buff, int avail) {
  int rv = gzread(gz, buff, avail);
  CHECK(rv >= 0);
  return rv;
}

IStreamGz::IStreamGz(const string &fname) {
  gz = gzopen(fname.c_str(), "rb");
  CHECK(gz);
}
IStreamGz::~IStreamGz() {
  gzclose(gz);
}

void OStreamGz::write_worker(const char *buff, int avail) {
  int rv = gzwrite(gz, buff, avail);
  CHECK(rv == avail);
}

OStreamGz::OStreamGz(const string &fname) {
  gz = gzopen(fname.c_str(), "wb");
  CHECK(gz);
}
OStreamGz::~OStreamGz() {
  gzclose(gz);
}
