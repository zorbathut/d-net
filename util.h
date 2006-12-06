#ifndef DNET_UTIL
#define DNET_UTIL

#include <string>
#include <vector>

using namespace std;

extern bool ffwd;

/*************
 * Text processing
 */

#ifdef printf
#define PFDEFINED
#undef printf
#endif

string StringPrintf(const char *bort, ...) __attribute__((format(printf,1,2)));

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

  long long money;

public:
  string textual() const;

  long long value() const;

  Money();
  explicit Money(long long in);
};

Money operator+(const Money &lhs, const Money &rhs);
Money operator-(const Money &lhs, const Money &rhs);

Money operator*(const Money &lhs, int rhs);
Money operator*(const Money &lhs, float rhs);
int operator/(const Money &lhs, const Money &rhs);
Money operator/(const Money &lhs, int rhs);
Money operator/(const Money &lhs, float rhs);
int operator%(const Money &lhs, int rhs);

const Money &operator+=(Money &lhs, const Money &rhs);
const Money &operator-=(Money &lhs, const Money &rhs);

bool operator==(const Money &lhs, const Money &rhs);
bool operator<(const Money &lhs, const Money &rhs);
bool operator<=(const Money &lhs, const Money &rhs);
bool operator>(const Money &lhs, const Money &rhs);
bool operator>=(const Money &lhs, const Money &rhs);

Money moneyFromString(const string &rhs);

/*************
 * Misc
 */

int modurot(int val, int mod);

namespace detail {
  template <typename type, ::std::size_t size> char (&array_size_impl(type(&)[size]))[size];
}
#define ARRAY_SIZE(array) sizeof(::detail::array_size_impl(array))

#define VECTORIZE(array) vector<int>((array), (array) + ARRAY_SIZE(array))

string rawstrFromFloat(float x);
float floatFromString(const string &x);

template <typename T> vector<T*> ptrize(vector<T> *vt) {
  vector<T*> rv;
  for(int i = 0; i < vt->size(); i++)
    rv.push_back(&(*vt)[i]);
  return rv;
}

template <typename T> vector<const T*> ptrize(const vector<T> &vt) {
  vector<const T*> rv;
  for(int i = 0; i < vt.size(); i++)
    rv.push_back(&vt[i]);
  return rv;
}

template<typename T, typename U> T clamp(T x, U min, U max) {
  //CHECK(min <= max);
  if(x < min)
    return min;
  if(x > max)
    return max;
  return x;
}

#endif
