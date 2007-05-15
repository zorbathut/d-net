
#include "util.h"

#include "debug.h"

#include <vector>
#include <cmath>
#include <sstream>
#include <stdarg.h>

using namespace std;

bool ffwd = false;

static bool inthread = false;

string StringPrintf(const char *bort, ...) {
  CHECK(!inthread);
  inthread = true;
  
  static vector< char > buf(2);
  va_list args;

  int done = 0;
  bool noresize = false;
  do {
    if(done && !noresize)
      buf.resize(buf.size() * 2);
    va_start(args, bort);
    done = vsnprintf(&(buf[0]), buf.size() - 1,  bort, args);
    if(done >= (int)buf.size()) {
      CHECK(noresize == false);
      CHECK(buf[buf.size() - 2] == 0);
      buf.resize(done + 2);
      done = -1;
      noresize = true;
    } else {
      CHECK(done < (int)buf.size());
    }
    va_end(args);
  } while(done == buf.size() - 1 || done == -1);

  CHECK(done < (int)buf.size());

  string rv = string(buf.begin(), buf.begin() + done);
  
  CHECK(inthread);
  inthread = false;
  
  return rv;
};

string stringFromLongdouble(long double x) {
  stringstream str;
  str << x;
  return str.str();
}

string prettyFloatFormat(float v) {
  if(v >= 10000 && v < 100000) {
    string borf = StringPrintf("%f", v);
    return string(borf.begin(), borf.begin() + 5);
  }
  if(!(0.01 <= v && v < 10000) && v != 0.0) {
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
 * Cashola
 */

string Money::textual() const {
  string text;
  long long moneytmp = money;
  CHECK(moneytmp >= 0);
  while(moneytmp) {
    text.push_back(moneytmp % 10 + '0');
    moneytmp /= 10;
  }
  if(!text.size())
    text = "0";
  
  int ks = 0;
  while(text.size() >= 6 && count(text.begin(), text.begin() + 3, '0') == 3) {
    text.erase(text.begin(), text.begin() + 3);
    ks++;
  }
  
  reverse(text.begin(), text.end());
  
  if(ks == 0)
    ;
  else if(ks == 1)
    text += " K";
  else if(ks == 2)
    text += " M";
  else if(ks == 3)
    text += " G";
  else if(ks == 4)
    text += " T";
  else
    CHECK(0);
  
  return text;
}

Money::Money() { };
Money::Money(long long in) { money = in; };

long long Money::value() const { return money; };
float Money::toFloat() const { return (float)money; };

Money operator+(const Money &lhs, const Money &rhs) {
  return Money(lhs.value() + rhs.value()); }
 Money operator-(const Money &lhs, const Money &rhs) {
  return Money(lhs.value() - rhs.value()); }

Money operator*(const Money &lhs, float rhs) {
  return Money((long long)(lhs.value() * rhs)); }
Money operator*(const Money &lhs, int rhs) {
  return Money(lhs.value() * rhs); }
int operator/(const Money &lhs, const Money &rhs) {
  return int(min(lhs.value() / rhs.value(), 2000000000LL)); }
Money operator/(const Money &lhs, int rhs) {
  return Money(lhs.value() / rhs); }
Money operator/(const Money &lhs, float rhs) {
  return Money((long long)(lhs.value() / rhs)); }

const Money &operator+=(Money &lhs, const Money &rhs) {
  lhs = lhs + rhs; return lhs; }
const Money &operator-=(Money &lhs, const Money &rhs) {
  lhs = lhs - rhs; return lhs; }

bool operator==(const Money &lhs, const Money &rhs) {
  return lhs.value() == rhs.value(); }
bool operator!=(const Money &lhs, const Money &rhs) {
  return lhs.value() != rhs.value(); }
bool operator<(const Money &lhs, const Money &rhs) {
  return lhs.value() < rhs.value(); }
bool operator<=(const Money &lhs, const Money &rhs) {
  return lhs < rhs || lhs == rhs; }
bool operator>(const Money &lhs, const Money &rhs) {
  return rhs < lhs; }
bool operator>=(const Money &lhs, const Money &rhs) {
  return rhs <= lhs; }

Money moneyFromString(const string &rhs) {
  CHECK(rhs.size());
  for(int i = 0; i < rhs.size() - 1; i++)
    CHECK(isdigit(rhs[i]));
  CHECK(isdigit(rhs[rhs.size() - 1]) || tolower(rhs[rhs.size() - 1]) == 'k' || tolower(rhs[rhs.size() - 1]) == 'm' || tolower(rhs[rhs.size() - 1]) == 'g');
  
  Money accum = Money(0);
  
  for(int i = 0; i < rhs.size(); i++) {
    if(isdigit(rhs[i])) {
      accum = accum * 10;
      accum = accum + Money(rhs[i] - '0');
    }
  }
  
  if(tolower(rhs[rhs.size() - 1]) == 'k') {
    accum = accum * 1000;
  } else if(tolower(rhs[rhs.size() - 1]) == 'm') {
    accum = accum * 1000000;
  } else if(tolower(rhs[rhs.size() - 1]) == 'g') {
    accum = accum * 1000000000;
  }
  
  return accum;
}

// It's made of money!

/*************
 * Misc
 */

int modurot(int val, int mod) {
  if(val < 0)
    val += abs(val) / mod * mod + mod;
  return val % mod;
}

void checkEndian() {
  float j = 12;
  CHECK(reinterpret_cast<unsigned char*>(&j)[3] == 65);
}

string rawstrFromFloat(float x) {
  // This is awful.
  CHECK(sizeof(x) == 4);
  CHECK(numeric_limits<float>::is_iec559);  // yayz
  checkEndian();
  unsigned char *dat = reinterpret_cast<unsigned char*>(&x);
  string beef;
  for(int i = 0; i < 4; i++)
    beef += StringPrintf("%02x", dat[i]);
  CHECK(beef.size() == 8);
  return beef;
}

float floatFromString(const string &x) {
  // This is also awful.
  CHECK(sizeof(float) == 4);
  CHECK(numeric_limits<float>::is_iec559); // wootz
  checkEndian();
  CHECK(x.size() == 8);
  float rv;
  unsigned char *dat = reinterpret_cast<unsigned char*>(&rv);
  for(int i = 0; i < 4; i++) {
    int v;
    CHECK(sscanf(x.c_str() + i * 2, "%2x", &v));
    CHECK(v >= 0 && v < 256);
    dat[i] = v;
  }
  CHECK(rawstrFromFloat(rv) == x);
  return rv;
}

string roman_number(int rid) {
  CHECK(rid >= 0);
  rid++; // okay this is kind of grim
  CHECK(rid < 40); // lazy
  string rv;
  while(rid >= 10) {
    rid -= 10;
    rv += "X";
  }
  
  if(rid == 9) {
    rid -= 9;
    rv += "IX";
  }
  
  if(rid >= 5) {
    rid -= 5;
    rv += "V";
  }
  
  if(rid == 4) {
    rid -= 4;
    rv += "IV";
  }
  
  rv += string(rid, 'I');
  
  return rv;
}

int roman_max() {
  return 38;
}

bool withinEpsilon(float a, float b, float e) {
  CHECK(e >= 0);
  if(a == b)
    return true; // I mean even if they're both 0
  float diff = a / b;
  if(diff < 0)
    return false; // it's not even the same *sign*
  if(abs(diff) < 1.0)
    diff = 1 / diff;
  return 1 - diff <= e;
}
