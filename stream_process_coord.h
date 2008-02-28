#ifndef DNET_STREAM_PROCESS_COORD
#define DNET_STREAM_PROCESS_COORD

#include "stream_process_primitive.h"
#include "coord.h"

#include <vector>

using namespace std;

template<> struct IStreamReader<Coord> { static void read(IStream *istr, Coord *storage) { long long v; istr->read(&v); *storage = coordExplicit(v); } };
template<> struct OStreamWriter<Coord> { static void write(OStream *istr, const Coord &storage) { istr->write(storage.raw()); } };

#endif
