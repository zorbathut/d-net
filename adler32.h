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
inline void adler(Adler32 *adl, long long val) { adl->addBytes(&val, sizeof(val)); }
inline void adler(Adler32 *adl, unsigned long long val) { adl->addBytes(&val, sizeof(val)); }

void adler(Adler32 *adl, float val);  // DO NOT IMPLEMENT

class AdlerIgnore {
public:
  AdlerIgnore();
  ~AdlerIgnore();
};

#define reg_adler(x) reg_adler_ul((x).output())
#define reg_adler_intermed(x) reg_adler((x))
#define reg_adler_ul(x) reg_adler_ul_worker((x), __FILE__, __LINE__)
#define reg_adler_ul_msg(x, y) reg_adler_ul_worker((x), __FILE__, __LINE__, y)
void reg_adler_ul_worker(unsigned long unl, const char *file, int line, const char *msg = NULL);

void reg_adler_ref_start();
void reg_adler_ref_item(unsigned long unl);

void reg_adler_ref_nullity();

int ret_adler_ref_count();
unsigned long ret_adler_ref();
void ret_adler_ref_clear();

#endif
