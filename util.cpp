
#include "util.h"

#include "parse.h"

#include <cmath>
#include <sstream>

#include <stdarg.h>

using namespace std;

bool ffwd = false;

string StringPrintf( const char *bort, ... ) {

  static vector< char > buf(2);
  va_list args;

  int done = 0;
  do {
    if( done )
      buf.resize( buf.size() * 2 );
    va_start( args, bort );
    done = vsnprintf( &(buf[ 0 ]), buf.size() - 1,  bort, args );
    CHECK( done < (int)buf.size() );
    va_end( args );
  } while( done == buf.size() - 1 || done == -1);

  CHECK( done < (int)buf.size() );

  return string(buf.begin(), buf.begin() + done);

};

string stringFromLongdouble(long double x) {
  stringstream str;
  str << x;
  return str.str();
}

string prettyFloatFormat(float v) {
  if(!(1 <= v && v < 10000)) {
    dprintf("err: %f\n", v);
  }
  string borf = StringPrintf("%.4f", v);
  int digic = 0;
  int cps = 0;
  while(digic != 4 && cps < borf.size()) {
    if(isdigit(borf[cps]))
      digic++;
    cps++;
  }
  return string(borf.begin(), borf.begin() + cps);
}

/*************
 * Matrixtastic
 */

void Transform2d::hflip() {
  for(int i = 0; i < 3; i++)
    m[0][i] *= -1;
}
void Transform2d::vflip() {
  for(int i = 0; i < 3; i++)
    m[1][i] *= -1;
}
void Transform2d::dflip() {
  for(int i = 0; i < 3; i++)
    swap(m[0][i], m[1][i]);
}

float Transform2d::det() {
  float rv = 0;
  for(int x = 0; x < 3; x++) {
    float tv = m[x][0] * detchunk(x, 0);
    if(x % 2)
      tv = -tv;
    rv += tv;
  }
  return rv;
}

vector<int> allExcept(int t) {
  CHECK(t >= 0 && t < 3);
  vector<int> rv;
  for(int i = 0; i < 3; i++)
    rv.push_back(i);
  rv.erase(find(rv.begin(), rv.end(), t));
  CHECK(rv.size() == 2);
  return rv;
}

float Transform2d::detchunk(int x, int y) {
  // this code sucks.
  vector<int> xv = allExcept(x);
  vector<int> yv = allExcept(y);
  CHECK(xv.size() == 2 && yv.size() == 2);
  return m[xv[0]][yv[0]] * m[xv[1]][yv[1]] - m[xv[0]][yv[1]] * m[xv[1]][yv[0]];
}

void Transform2d::invert() {
  // hahahahahhahah.
  Transform2d res;
  for(int x = 0; x < 3; x++) {
    for(int y = 0; y < 3; y++) {
      res.m[x][y] = detchunk(y, x) / det();
      if((x + y) % 2)
        res.m[x][y] = -res.m[x][y];
    }
  }
  *this = res;
}

float Transform2d::mx(float x, float y) const {
  float ox = 0.0;
  ox += m[0][0] * x;
  ox += m[0][1] * y;
  ox += m[0][2];
  return ox;
}

float Transform2d::my(float x, float y) const {
  float oy = 0.0;    
  oy += m[1][0] * x;
  oy += m[1][1] * y;
  oy += m[1][2];
  return oy;
}

void Transform2d::transform(float *x, float *y) const {
  float px = *x;
  float py = *y;
  *x = mx(px, py);
  *y = my(px, py);
}

void Transform2d::display() const {
  for(int i = 0; i < 3; i++)
    dprintf("  %f %f %f\n", m[0][i], m[1][i], m[2][i]);
}

Transform2d::Transform2d() {
  for(int i = 0; i < 3; i++)
    for(int j = 0; j < 3; j++)
      m[i][j] = ( i == j );
}

Transform2d operator*(const Transform2d &lhs, const Transform2d &rhs) {
  Transform2d rv;
  for(int x = 0; x < 3; x++) {
    for(int y = 0; y < 3; y++) {
      rv.m[x][y] = 0;
      for(int z = 0; z < 3; z++) {
        rv.m[x][y] += lhs.m[z][y] * rhs.m[x][z];
      }
    }
  }
  return rv;
}

Transform2d &operator*=(Transform2d &lhs, const Transform2d &rhs) {
  lhs = lhs * rhs;
  return lhs;
}

Transform2d t2d_identity() {
  return Transform2d();
}
Transform2d t2d_flip(bool h, bool v, bool d) {
  Transform2d o;
  if(h)
    o.hflip();
  if(v)
    o.vflip();
  if(d)
    o.dflip();
  return o;
}
Transform2d t2d_rotate(float rads) {
  Transform2d o;
  o.m[0][0] = cos(rads);
  o.m[0][1] = sin(rads);
  o.m[1][0] = -sin(rads);
  o.m[1][1] = cos(rads);
  return o;
}

string Money::textual() const {
  return stringFromLongdouble(money);
}

float Money::toFloat() const {
  return money;
}

Money::Money() { };
Money::Money(float in) { money = floor(in + 0.5); };
Money::Money(int in) { money = in; };

long double Money::raw() const { return money; };
Money::Money(long double in) { money = floor(in + 0.5); };

Money operator+(const Money &lhs, const Money &rhs) {
  return Money(lhs.raw() + rhs.raw()); }
 Money operator-(const Money &lhs, const Money &rhs) {
  return Money(lhs.raw() - rhs.raw()); }

Money operator*(const Money &lhs, float rhs) {
  return Money(lhs.raw() * rhs); }
Money operator*(const Money &lhs, int rhs) {
  return Money(lhs.raw() * rhs); }
int operator/(const Money &lhs, const Money &rhs) {
  return int(min(lhs.raw() / rhs.raw(), 2000000000.L)); }
Money operator/(const Money &lhs, int rhs) {
  return Money(lhs.raw() / rhs); }
Money operator/(const Money &lhs, float rhs) {
  return Money(lhs.raw() / rhs); }

const Money &operator+=(Money &lhs, const Money &rhs) {
  lhs = lhs + rhs; return lhs; }
const Money &operator-=(Money &lhs, const Money &rhs) {
  lhs = lhs - rhs; return lhs; }

bool operator==(const Money &lhs, const Money &rhs) {
  return lhs.raw() == rhs.raw(); }
bool operator<(const Money &lhs, const Money &rhs) {
  return lhs.raw() < rhs.raw(); }
bool operator<=(const Money &lhs, const Money &rhs) {
  return lhs < rhs || lhs == rhs; }
bool operator>(const Money &lhs, const Money &rhs) {
  return rhs < lhs; }
bool operator>=(const Money &lhs, const Money &rhs) {
  return rhs <= lhs; }

Money moneyFromString(const string &rhs) {
  CHECK(rhs.size() < 10);
  return Money(atoi(rhs.c_str()));
}

Color::Color() { };
Color::Color(float in_r, float in_g, float in_b) :
  r(in_r), g(in_g), b(in_b) { };

Color colorFromString(const string &str) {
  if(str.size() == 3) {
    // 08f style color
    return Color(fromHex(str[0]) / 15., fromHex(str[1]) / 15., fromHex(str[2]) / 15.);
  } else {
    // 1.0, 0.3, 0.194 style color
    vector<string> toki = tokenize(str, " ,");
    CHECK(toki.size() == 3);
    return Color(atof(toki[0].c_str()), atof(toki[1].c_str()), atof(toki[2].c_str()));
  }
}

Color operator+(const Color &lhs, const Color &rhs) {
  return Color(lhs.r + rhs.r, lhs.g + rhs.g, lhs.b + rhs.b);
}
Color operator*(const Color &lhs, float rhs) {
  return Color(lhs.r * rhs, lhs.g * rhs, lhs.b * rhs);
}
Color operator/(const Color &lhs, float rhs) {
  return Color(lhs.r / rhs, lhs.g / rhs, lhs.b / rhs);
}

const Color &operator+=(Color &lhs, const Color &rhs) {
  lhs = lhs + rhs;
  return lhs;
}
