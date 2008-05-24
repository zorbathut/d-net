
#include "stream_file.h"

#include "debug.h"

int IStreamFile::read_worker(char *buff, int avail) {
  CHECK(file);
  CHECK(avail >= 0);
  int fr = fread(buff, 1, avail, file);
  return fr;
}

IStreamFile::operator void*() const { return file; }

IStreamFile::IStreamFile(const string &fname) : file(fopen(fname.c_str(), "rb")) { };
IStreamFile::~IStreamFile() { if(file) fclose(file); };

void OStreamFile::write_worker(const char *buff, int avail) {
  CHECK(file);
  CHECK(avail >= 0);
  int fr = fwrite(buff, 1, avail, file);
  CHECK(fr == avail);
}

OStreamFile::operator void*() const { return file; }

OStreamFile::OStreamFile(const string &fname) : file(fopen(fname.c_str(), "wb")) { };
OStreamFile::~OStreamFile() { flush(); if(file) fclose(file); };
