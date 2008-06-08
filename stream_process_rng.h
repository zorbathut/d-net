#ifndef DNET_STREAM_PROCESS_RNG
#define DNET_STREAM_PROCESS_RNG

#include "rng.h"
#include "stream_process_primitive.h"

using namespace std;

template<> struct IStreamReader<RngSeed> { static bool read(IStream *istr, RngSeed *storage) { unsigned long v; if(istr->tryRead(&v)) return true; *storage = RngSeed(v); return false; } };
template<> struct OStreamWriter<RngSeed> { static void write(OStream *istr, const RngSeed &storage) { istr->write(storage.getSeed()); } };

#endif
