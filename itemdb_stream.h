#ifndef DNET_ITEMDB_STREAM
#define DNET_ITEMDB_STREAM

#include "itemdb.h"
#include "stream.h"
using namespace std;

bool stream_read(IStream *istr, FileShopcache *storage);
bool stream_read(IStream *istr, FileShopcache::Entry *storage);

void stream_write(OStream *ostr, const FileShopcache &storage);
void stream_write(OStream *ostr, const FileShopcache::Entry &storage);

#endif
