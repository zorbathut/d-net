#ifndef DNET_STREAM_PROCESS_RNG
#define DNET_STREAM_PROCESS_RNG

#include "stream_process_primitive.h"
#include "rng.h"

#include <vector>

using namespace std;

template<> struct IStreamReader<RngSeed> { static void read(IStream *istr, RngSeed *storage) { unsigned long v; istr->read(&v); *storage = RngSeed(v); } };
template<> struct OStreamWriter<RngSeed> { static void write(OStream *istr, const RngSeed &storage) { istr->write(storage.getSeed()); } };

#endif
