#ifndef DNET_UTIL
#define DNET_UTIL

#include <string>

using namespace std;

extern bool ffwd;

/*************
 * Text processing
 */

#ifdef printf
#define PFDEFINED
#undef printf
#endif

string StringPrintf( const char *bort, ... ) __attribute__((format(printf,1,2)));

#ifdef PFDEFINED
#define printf FAILURE
#undef PFDEFINED
#endif

string stringFromLongdouble(long double ld);
string prettyFloatFormat(float v);  // this formats it to have four numeric digits. requires 1 <= n < 10000.

/*************
 * Matrixtastic
 */

class Transform2d {
public:
  float m[3][3];

  void hflip();
  void vflip();
  void dflip();
  
  float det();
  float detchunk(int x, int y);
  
  void invert();
  
  float mx(float x, float y) const;
  float my(float x, float y) const;
  
  void transform(float *x, float *y) const;
  
  void display() const;

  Transform2d();
};

Transform2d operator*(const Transform2d &lhs, const Transform2d &rhs);
Transform2d &operator*=(Transform2d &lhs, const Transform2d &rhs);

Transform2d t2d_identity();
Transform2d t2d_flip(bool h, bool v, bool d);
Transform2d t2d_rotate(float rads);

/*************
 * Cashola
 */

class Money { // Like my hat?
private:

  long double money;

public:
  string textual() const;

  float toFloat() const;

  Money();
  explicit Money(float in);
  explicit Money(int in);

  // These should not be used by anything outside util.cpp.
  long double raw() const;
  explicit Money(long double in);
};  // It's made of money!

Money operator+(const Money &lhs, const Money &rhs);
Money operator-(const Money &lhs, const Money &rhs);

Money operator*(const Money &lhs, int rhs);
Money operator*(const Money &lhs, float rhs);
int operator/(const Money &lhs, const Money &rhs);
Money operator/(const Money &lhs, int rhs);
Money operator/(const Money &lhs, float rhs);

const Money &operator+=(Money &lhs, const Money &rhs);
const Money &operator-=(Money &lhs, const Money &rhs);

bool operator==(const Money &lhs, const Money &rhs);
bool operator<(const Money &lhs, const Money &rhs);
bool operator<=(const Money &lhs, const Money &rhs);
bool operator>(const Money &lhs, const Money &rhs);
bool operator>=(const Money &lhs, const Money &rhs);

Money moneyFromString(const string &rhs);

/*************
 * Color struct
 */
 
struct Color {
public:
  float r, g, b;

  Color();
  Color(float in_r, float in_g, float in_b);
};

Color colorFromString(const string &str);

Color operator+(const Color &lhs, const Color &rhs);
Color operator*(const Color &lhs, float rhs);
Color operator/(const Color &lhs, float rhs);

const Color &operator+=(Color &lhs, const Color &rhs);

/*************
 * Smart pointer
 */

template<typename T> class smart_ptr {
private:
  T *ptr;
  int *ct;

public:
  void reset() {
    if(ptr) {
      (*ct)--;
      if(*ct == 0) {
        delete ct;
        delete ptr;
      }
      ct = NULL;
      ptr = NULL;
    }
  }
  void reset(T *pt) {
    reset();
    if(pt) {
      ptr = pt;
      ct = new int(1);
    }
  }
  
  T *get() {
    CHECK(ptr);
    return ptr;
  }
  const T *get() const {
    CHECK(ptr);
    return ptr;
  }

  T *operator->() {
    return get();
  }
  const T *operator->() const {
    return get();
  }

  smart_ptr<T> &operator=(const smart_ptr<T> &x) {
    reset();
    if(x.ptr) {
      ptr = x.ptr;
      ct = x.ct;
      (*ct)++;
    }
    return *this;
  }

  smart_ptr() {
    ct = NULL;
    ptr = NULL;
  }
  smart_ptr(const smart_ptr<T> &x) {
    ct = NULL;
    ptr = NULL;
    *this = x;
  }
  explicit smart_ptr<T>(T *pt) {
    ct = NULL;
    ptr = NULL;
    reset(pt);
  }
  ~smart_ptr() {
    reset();
  }
};

#endif
