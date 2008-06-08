
#include "itemdb_stream.h"
#include "stream_process_string.h"
#include "stream_process_utility.h"
#include "stream_process_vector.h"
using namespace std;


bool stream_read(IStream *istr, FileShopcache *storage) {
  if(istr->tryRead(&storage->entries)) return true;
  if(istr->tryRead(&storage->cycles)) return true;
  if(istr->tryRead(&storage->damageframes)) return true;
  return false;
}

bool stream_read(IStream *istr, FileShopcache::Entry *storage) {
  if(istr->tryRead(&storage->warhead)) return true;
  if(istr->tryRead(&storage->count)) return true;
  if(istr->tryRead(&storage->mult)) return true;
  if(istr->tryRead(&storage->impact)) return true;
  if(istr->tryRead(&storage->adjacencies)) return true;
  return false;
}

void stream_write(OStream *ostr, const FileShopcache &storage) {
  ostr->write(storage.entries);
  ostr->write(storage.cycles);
  ostr->write(storage.damageframes);
}

void stream_write(OStream *ostr, const FileShopcache::Entry &storage) {
  ostr->write(storage.warhead);
  ostr->write(storage.count);
  ostr->write(storage.mult);
  ostr->write(storage.impact);
  ostr->write(storage.adjacencies);
}
