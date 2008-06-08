#ifndef DNET_STREAM_PROCESS_COORD
#define DNET_STREAM_PROCESS_COORD

#include "coord.h"
#include "stream_process_primitive.h"

using namespace std;

template<> struct IStreamReader<Coord> { static bool read(IStream *istr, Coord *storage) { long long v; if(istr->tryRead(&v)) return true; *storage = coordExplicit(v); return false; } };
template<> struct OStreamWriter<Coord> { static void write(OStream *istr, const Coord &storage) { istr->write(storage.raw()); } };

#endif
