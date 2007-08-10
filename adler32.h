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

  int count;
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

#define reg_adler(x) reg_adler_ul((x).output())
#define reg_adler_intermed(x) reg_adler((x))
#define reg_adler_ul(x) reg_adler_ul_worker((x), __FILE__, __LINE__)
void reg_adler_ul_worker(unsigned long unl, const char *file, int line);

void reg_adler_ref_start();
void reg_adler_ref_item(unsigned long unl);

int ret_adler_ref_count();
unsigned long ret_adler_ref();
void ret_adler_ref_clear();

#endif
