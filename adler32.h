#ifndef DNET_ADLER32
#define DNET_ADLER32

#include "debug.h"

#define MOD_ADLER 65521

class Adler32 {
public:
  Adler32();

  unsigned long output() const;

  void addByte(unsigned char x) {
    b += a += x;
    if(unlikely(count == 5540)) {
      a = (a & 0xffff) + (a >> 16) * (65536-MOD_ADLER);
      b = (b & 0xffff) + (b >> 16) * (65536-MOD_ADLER);
      count = 0;
    } else {
      count++;
    }
  }
  void addBytes(const void *x, int len) {
    const unsigned char *dv = (const unsigned char *)x;
    CHECK(len);
    do {
      int tlen = min(len, 5540 - count);
      len -= tlen;
      for(int i = 0; i < tlen; i++)
        b += a += *dv++;
      count += tlen;
      if(unlikely(count == 5540)) {
        a = (a & 0xffff) + (a >> 16) * (65536-MOD_ADLER);
        b = (b & 0xffff) + (b >> 16) * (65536-MOD_ADLER);
        count = 0;
      }
    } while(unlikely(len));
  }
  
private:
  unsigned int a;
  unsigned int b;

  int count;
};

inline void adler(Adler32 *adl, bool val) { adl->addByte(val); }
inline void adler(Adler32 *adl, char val) { adl->addByte(val); }
inline void adler(Adler32 *adl, unsigned char val) { adl->addByte(val); }
inline void adler(Adler32 *adl, int val) { adl->addBytes(&val, sizeof(val)); }
inline void adler(Adler32 *adl, unsigned int val) { adl->addBytes(&val, sizeof(val)); }
inline void adler(Adler32 *adl, long val) { adl->addBytes(&val, sizeof(val)); }
inline void adler(Adler32 *adl, unsigned long val) { adl->addBytes(&val, sizeof(val)); }
inline void adler(Adler32 *adl, long long val) { adl->addBytes(&val, sizeof(val)); }
inline void adler(Adler32 *adl, unsigned long long val) { adl->addBytes(&val, sizeof(val)); }
inline void adler(Adler32 *adl, const char *val) { adl->addBytes(&val, strlen(val)); }
template<typename T> void adler(Adler32 *adl, T *val) { val->lolzfail(); }
template<typename T> void adler(Adler32 *adl, const T *val) { val->lolzfail(); } // these are intentionally implemented as failures, so these don't implicitly cast to bool

//void adler(Adler32 *adl, float val, int foo);  // DO NOT IMPLEMENT

#endif
