#ifndef DNET_ADLER32
#define DNET_ADLER32

class Adler32 {
public:
  Adler32();

  unsigned long output() const;

  void addByte(unsigned char x);
  void addBytes(const void *x, int len);
private:
  unsigned int a;
  unsigned int b;
};

class Coord;
class Coord2;
class Coord4;

void adler(Adler32 *adl, bool val);
void adler(Adler32 *adl, char val);
void adler(Adler32 *adl, unsigned char val);
void adler(Adler32 *adl, int val);
void adler(Adler32 *adl, unsigned int val);
void adler(Adler32 *adl, long long val);
void adler(Adler32 *adl, unsigned long long val);
void adler(Adler32 *adl, const Coord &val);
void adler(Adler32 *adl, const Coord2 &val);
void adler(Adler32 *adl, const Coord4 &val);

void adler(Adler32 *adl, float val);  // DO NOT IMPLEMENT

#endif
