
#include "regex.h"
#include "debug.h"
using namespace std;


boost::smatch match(const string &in, const string &expression) {
  boost::regex exp("([0-9]+) (.*)");
  boost::smatch rv;
  CHECK(boost::regex_match(in, rv, exp));
  return rv;
}
