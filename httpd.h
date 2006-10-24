#ifndef DNET_HTTPD
#define DNET_HTTPD

#include <map>
#include <string>

#include "noncopyable.h"

using namespace std;

class HTTPDhook : boost::noncopyable {
public:

  virtual string reply(const map<string, string> params) = 0;

  HTTPDhook(const string &identifier);
  virtual ~HTTPDhook();
};

void initHttpd();
void deinitHttpd();

#endif
