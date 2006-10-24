
#include "httpd.h"

#include "socket.h"
#include "debug.h"
#include "args.h"

HTTPDhook::HTTPDhook(const string &identifier) { dprintf("Hook %s\n", identifier.c_str()); };
HTTPDhook::~HTTPDhook() { };

class HTTPD {
  
  Listener listener;
  
public:

  void process() {
    while(1) {
      smart_ptr<Socket> ptr = listener.consumeNewConnection();
      if(ptr.empty())
        break;
      dprintf("New connection");
    }
  }
  
  HTTPD(int port) : listener(port) { };
  ~HTTPD() { };
};

HTTPD *httpd = NULL;

class ArgsHTTPD : public HTTPDhook {
public:

  string reply(const map<string, string> params) {
    return "args args args";
  }

  ArgsHTTPD() : HTTPDhook("args") { };
};

ArgsHTTPD *httpdargs = NULL;

DEFINE_int(httpd_port, -1, "Port for HTTPD control interface (-1 to disable)");

void initHttpd() {
  CHECK(!httpd);
  CHECK(!httpdargs);
  if(FLAGS_httpd_port != -1) {
    httpd = new HTTPD(FLAGS_httpd_port);
    httpdargs = new ArgsHTTPD;
  }
}

void tickHttpd() {
  StackString sst("httpdtick");
  if(httpd)
    httpd->process();
}

void deinitHttpd() {
  delete httpdargs;
  delete httpd;
  httpdargs = NULL;
  httpd = NULL;
}
