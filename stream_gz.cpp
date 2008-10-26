
#include "stream_gz.h"

#include "util.h"

#include <vector>
#include <zlib.h>

using namespace std;

vector<OStreamGz *> activeostreamgz;

void flushOstreamGzOnCrash() {
  for(int i = 0; i < activeostreamgz.size(); i++) {
    activeostreamgz[i]->flush_and_terminate();
  }
  activeostreamgz.clear();
}

int IStreamGz::read_worker(char *buff, int avail) {
  int rv = gzread(gz, buff, avail);
  CHECK(rv >= 0);
  return rv;
}

IStreamGz::operator const void*() const { return this; }

IStreamGz::IStreamGz(const string &fname) {
  gz = gzopen(fname.c_str(), "rb");
  CHECK(gz);
}
IStreamGz::~IStreamGz() {
  gzclose(gz);
}

void OStreamGz::flush_and_terminate() {
  dprintf("flush-and-terminate!\n");
  CHECK(gz);
  flush();
  gzclose(gz);
  gz = NULL;
}

void OStreamGz::write_worker(const char *buff, int avail) {
  CHECK(gz);
  int rv = gzwrite(gz, buff, avail);
  CHECK(rv == avail);
}

OStreamGz::operator const void*() const { return this; }

OStreamGz::OStreamGz(const string &fname, int level) {
  CHECK(level >= 0 && level <= 9);
  gz = gzopen(fname.c_str(), StringPrintf("wb%d", level).c_str());
  CHECK(gz);
  activeostreamgz.push_back(this);
  registerCrashFunction(flushOstreamGzOnCrash);
}
OStreamGz::~OStreamGz() {
  unregisterCrashFunction(flushOstreamGzOnCrash);
  flush();
  if(gz)
    gzclose(gz);
  activeostreamgz.erase(find(activeostreamgz.begin(), activeostreamgz.end(), this));
}
