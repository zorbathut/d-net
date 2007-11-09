
#include "itemdb_stream.h"

#include "stream_process_vector.h"
#include "stream_process_utility.h"
#include "stream_process_string.h"

void stream_read(IStream *istr, FileShopcache *storage) {
  istr->read(&storage->entries);
  istr->read(&storage->cycles);
  istr->read(&storage->damageframes);
}

void stream_read(IStream *istr, FileShopcache::Entry *storage) {
  istr->read(&storage->warhead);
  istr->read(&storage->count);
  istr->read(&storage->mult);
  istr->read(&storage->impact);
  istr->read(&storage->adjacencies);
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
