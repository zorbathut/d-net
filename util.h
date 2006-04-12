#ifndef DNET_UTIL
#define DNET_UTIL

#include <utility>
#include <algorithm>
#include <vector>
#include <cmath>

using namespace std;

#include "const.h"
#include "debug.h"

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
int operator/(const Money &lhs, const Money &rhs);
Money operator/(const Money &lhs, int rhs);

const Money &operator+=(Money &lhs, const Money &rhs);
const Money &operator-=(Money &lhs, const Money &rhs);

bool operator==(const Money &lhs, const Money &rhs);
bool operator<(const Money &lhs, const Money &rhs);
bool operator<=(const Money &lhs, const Money &rhs);
bool operator>(const Money &lhs, const Money &rhs);
bool operator>=(const Money &lhs, const Money &rhs);

Money moneyFromString(const string &rhs);

#endif
