#ifndef DNET_REGEX
#define DNET_REGEX

#include <string>

#include <boost/regex.hpp>

using namespace std;

boost::smatch match(const string &in, const string &expression);

#endif
