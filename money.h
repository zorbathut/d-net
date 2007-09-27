#ifndef DNET_MONEY
#define DNET_MONEY

#include <string>
#include <vector>

using namespace std;

class Money { // Like my hat?
private:

  long long money;

public:
  
  enum { TEXT_NOABBREV = 1, TEXT_NORIGHTPAD = 4 };
  
  string textual(int flags = 0) const;

  long long value() const;
  float toFloat() const;

  Money();
  explicit Money(long long in);
};

Money operator+(const Money &lhs, const Money &rhs);
Money operator-(const Money &lhs, const Money &rhs);

Money operator*(const Money &lhs, int rhs);
Money operator*(const Money &lhs, double rhs);
int operator/(const Money &lhs, const Money &rhs);
Money operator/(const Money &lhs, int rhs);
Money operator/(const Money &lhs, double rhs);
int operator%(const Money &lhs, int rhs);

const Money &operator+=(Money &lhs, const Money &rhs);
const Money &operator-=(Money &lhs, const Money &rhs);

bool operator==(const Money &lhs, const Money &rhs);
bool operator!=(const Money &lhs, const Money &rhs);
bool operator<(const Money &lhs, const Money &rhs);
bool operator<=(const Money &lhs, const Money &rhs);
bool operator>(const Money &lhs, const Money &rhs);
bool operator>=(const Money &lhs, const Money &rhs);

Money moneyFromString(const string &rhs);
vector<string> formatMultiMoney(const vector<Money> &vm);

class Adler32;
void adler(Adler32 *adl, const Money &money);

#endif
