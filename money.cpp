
#include "money.h"

#include "adler32.h"

string Money::textual(int value) const {
  string text;
  long long moneytmp = money;
  CHECK(moneytmp >= 0);
  int dig = 0;
  while(moneytmp) {
    text.push_back(moneytmp % 10 + '0');
    moneytmp /= 10;
    dig++;
    if(moneytmp && dig >= 3 && dig % 3 == 0)
      text.push_back(',');
  }
  if(!text.size())
    text = "0";
  
  int ks = 0;
  if(!(value & TEXT_NOABBREV)) {
    while(text.size() >= 6 && text.substr(0, 4) == "000,") {
      text.erase(text.begin(), text.begin() + 4);
      ks++;
    }
  }
  
  reverse(text.begin(), text.end());
  
  if(ks == 0) {
    if(!(value & TEXT_NORIGHTPAD))
      text += " _";
  } else if(ks == 1) {
    text += " K";
  } else if(ks == 2) {
    text += " M";
  } else if(ks == 3) {
    text += " B";
  } else if(ks == 4) {
    text += " T";
  } else {
    CHECK(0);
  }
  
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

Money operator*(const Money &lhs, double rhs) {
  return Money((long long)(lhs.value() * rhs)); }
Money operator*(const Money &lhs, int rhs) {
  return Money(lhs.value() * rhs); }
int operator/(const Money &lhs, const Money &rhs) {
  return int(min(lhs.value() / rhs.value(), 2000000000LL)); }
Money operator/(const Money &lhs, int rhs) {
  return Money(lhs.value() / rhs); }
Money operator/(const Money &lhs, double rhs) {
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
  for(int i = 0; i < rhs.size() - 1; i++) {
    if(!isdigit(rhs[i])) {
      dprintf("%s isn't money!\n", rhs.c_str());
      CHECK(0);
    }
  }
  CHECK(isdigit(rhs[rhs.size() - 1]) || tolower(rhs[rhs.size() - 1]) == 'k' || tolower(rhs[rhs.size() - 1]) == 'm' || tolower(rhs[rhs.size() - 1]) == 'g');
  
  Money accum = Money(0);
  
  for(int i = 0; i < rhs.size(); i++) {
    if(isdigit(rhs[i])) {
      Money laccum = accum;
      accum = accum * 10;
      CHECK(accum >= laccum);
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

void adler(Adler32 *adl, const Money &money) {
  adler(adl, money.value());
}
