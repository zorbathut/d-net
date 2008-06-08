
#include "money.h"

#include "adler32.h"

using namespace std;

string getBaseText(const Money &mon) {
  string text;
  long long moneytmp = mon.value();
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
  return text;
}

int doabbrev(const string &in_text, int params) {
  string text = in_text;
  int rv = 0;
  if(!(params & Money::TEXT_NOABBREV)) {
    while((text.size() >= 6 && text.substr(0, 4) == "000,") || (text.size() >= 7 && (params & Money::TEXT_ROUND))) {
      text.erase(text.begin(), text.begin() + 4);
      rv++;
    }
  }
  return rv;
}

string addsuffix(const string &in_a, int ks, int params) {
  string in = in_a;
  bool inexact = false;
  for(int i = 0; i < ks; i++) {
    CHECK(in.size() >= 6);
    if(in.substr(0, 4) != "000,") {
      CHECK(params & Money::TEXT_ROUND);
      inexact = true;
    }
    in.erase(in.begin(), in.begin() + 4);
  }
  
  if(inexact)
    in += "~";
  
  reverse(in.begin(), in.end());
  
  if(ks == 0 && !(params & Money::TEXT_NORIGHTPAD)) {
    in += " _";
  } else if(ks == 0 && (params & Money::TEXT_NORIGHTPAD)) {
  } else if(ks == 1) {
    in += " K";
  } else if(ks == 2) {
    in += " M";
  } else if(ks == 3) {
    in += " B";
  } else if(ks == 4) {
    in += " T";
  } else {
    CHECK(0);
  }
  return in;
}

string Money::textual(int value) const {
  string text = getBaseText(*this);
  
  int ks = doabbrev(text, value);
  
  return addsuffix(text, ks, value);
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

vector<string> formatMultiMoney(const vector<Money> &vm) {
  if(!vm.size())
    return vector<string>();
  
  vector<string> oot;
  for(int i = 0; i < vm.size(); i++)
    oot.push_back(getBaseText(vm[i]));
  
  int tx = 1000;
  for(int i = 0; i < oot.size(); i++)
    tx = min(tx, doabbrev(oot[i], 0));
  
  for(int i = 0; i < oot.size(); i++)
    oot[i] = addsuffix(oot[i], tx, 0);
  
  return oot;
}

void adler(Adler32 *adl, const Money &money) {
  adler(adl, money.value());
}
