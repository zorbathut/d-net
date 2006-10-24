
#include "httpd.h"

#include "socket.h"
#include "debug.h"
#include "args.h"
#include "parse.h"

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
      string location = "";
      while(1) {
        string ins = ptr->receiveline();
        dprintf("%s\n", ins.c_str());
        if(ins.size() == 0)
          break;
        vector<string> tok = tokenize(ins, " ");
        if(tok[0] == "GET")
          location = tok[1];
      }
      CHECK(location.size());
      
      ptr->sendline("HTTP/1.1 200 OK");
      ptr->sendline("Content-type: text/html");
      ptr->sendline("");
      ptr->sendline("waffles are fucking awesome dude");
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
