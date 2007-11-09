#ifndef DNET_STREAM_PROCESS_PRIMITIVE
#define DNET_STREAM_PROCESS_PRIMITIVE

#include "stream.h"

#include <boost/static_assert.hpp>
#include <boost/detail/endian.hpp>

BOOST_STATIC_ASSERT(sizeof(char) == 1); // if this is wrong, we have problems
BOOST_STATIC_ASSERT(CHAR_BIT == 8);
#ifndef BOOST_LITTLE_ENDIAN
#error Little-endian required for this code at the moment, fix this at some point
#endif

BOOST_STATIC_ASSERT(sizeof(short) == 2);
BOOST_STATIC_ASSERT(sizeof(int) == 4);
BOOST_STATIC_ASSERT(sizeof(float) == 4);

template<> struct IStreamReader<char> { static void read(IStream *istr, char *storage) { istr->read((char*)storage, sizeof(*storage)); } };
template<> struct IStreamReader<unsigned char> { static void read(IStream *istr, unsigned char *storage) { istr->read((char*)storage, sizeof(*storage)); } };
template<> struct IStreamReader<short> { static void read(IStream *istr, short *storage) { istr->read((char*)storage, sizeof(*storage)); } };
template<> struct IStreamReader<unsigned short> { static void read(IStream *istr, unsigned short *storage) { istr->read((char*)storage, sizeof(*storage)); } };
template<> struct IStreamReader<int> { static void read(IStream *istr, int *storage) { istr->read((char*)storage, sizeof(*storage)); } };
template<> struct IStreamReader<unsigned int> { static void read(IStream *istr, unsigned int *storage) { istr->read((char*)storage, sizeof(*storage)); } };
template<> struct IStreamReader<float> { static void read(IStream *istr, float *storage) { istr->read((char*)storage, sizeof(*storage)); } };

template<> struct OStreamWriter<char> { static void write(OStream *istr, const char &storage) { istr->write((char*)&storage, sizeof(storage)); } };
template<> struct OStreamWriter<unsigned char> { static void write(OStream *istr, const unsigned char &storage) { istr->write((char*)&storage, sizeof(storage)); } };
template<> struct OStreamWriter<short> { static void write(OStream *istr, const short &storage) { istr->write((char*)&storage, sizeof(storage)); } };
template<> struct OStreamWriter<unsigned short> { static void write(OStream *istr, const unsigned short &storage) { istr->write((char*)&storage, sizeof(storage)); } };
template<> struct OStreamWriter<int> { static void write(OStream *istr, const int &storage) { istr->write((char*)&storage, sizeof(storage)); } };
template<> struct OStreamWriter<unsigned int> { static void write(OStream *istr, const unsigned int &storage) { istr->write((char*)&storage, sizeof(storage)); } };
template<> struct OStreamWriter<float> { static void write(OStream *istr, const float &storage) { istr->write((char*)&storage, sizeof(storage)); } };

#endif
