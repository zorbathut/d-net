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

BOOST_STATIC_ASSERT(sizeof(bool) == 1);
BOOST_STATIC_ASSERT(sizeof(short) == 2);
BOOST_STATIC_ASSERT(sizeof(int) == 4);
BOOST_STATIC_ASSERT(sizeof(long) == 4);
BOOST_STATIC_ASSERT(sizeof(long long) == 8);
BOOST_STATIC_ASSERT(sizeof(float) == 4);

#define PODTYPE(type) \
  template<> struct IStreamReader<type> { static bool read(IStream *istr, type *storage) { return istr->tryRead((char*)storage, sizeof(*storage)); } }; \
  template<> struct OStreamWriter<type> { static void write(OStream *istr, const type &storage) { istr->write((char*)&storage, sizeof(storage)); } };

PODTYPE(char);
PODTYPE(unsigned char);
PODTYPE(short);
PODTYPE(unsigned short);
PODTYPE(int);
PODTYPE(unsigned int);
PODTYPE(long);
PODTYPE(unsigned long);
PODTYPE(long long);
PODTYPE(unsigned long long);

PODTYPE(float);

// a little bit different because the internal representation of bool is less defined
template<> struct IStreamReader<bool> { static bool read(IStream *istr, bool *storage) { char tv; if(istr->tryRead(&tv)) return true; *storage = tv; return false; } };
template<> struct OStreamWriter<bool> { static void write(OStream *istr, const bool &storage) { istr->write((char)storage); } };

#endif
