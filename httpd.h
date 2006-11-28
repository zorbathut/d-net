#ifndef DNET_HTTPD
#define DNET_HTTPD

#include <map>
#include <string>

#include "boost/noncopyable.hpp"

using namespace std;

class HTTPDhook : boost::noncopyable {
  string identifier;
  
public:

  virtual string reply(const map<string, string> &params) = 0;

  HTTPDhook(const string &identifier);
  virtual ~HTTPDhook();
};

void initHttpd();
void tickHttpd();
void deinitHttpd();

#endif
