
#include "httpd.h"

#include "socket.h"
#include "debug.h"
#include "args.h"
#include "parse.h"
#include "util.h"

map<string, HTTPDhook *> &getHooks() {
  static map<string, HTTPDhook *> hooks;
  return hooks;
}

HTTPDhook::HTTPDhook(const string &identifier) : identifier(identifier) {
  CHECK(!getHooks().count(identifier));
  getHooks()[identifier] = this;
};
HTTPDhook::~HTTPDhook() {
  CHECK(getHooks().count(identifier));
  CHECK(getHooks()[identifier] == this);
  getHooks().erase(identifier);
};

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
      CHECK(location[0] == '/');
      location.erase(location.begin());
      
      if(!getHooks().count(location)) {
        ptr->sendline("HTTP/1.1 404 File Not Found");
        ptr->sendline("Content-type: text/html");
        ptr->sendline("");
        ptr->sendline("owned");
      } else {
        ptr->sendline("HTTP/1.1 200 OK");
        ptr->sendline("Content-type: text/html");
        ptr->sendline("");
        ptr->sendline(StringPrintf("Listing for /%s:<p>", location.c_str()));
        ptr->sendline(getHooks()[location]->reply(map<string, string>()));
      }
    }
  }
  
  HTTPD(int port) : listener(port) { };
  ~HTTPD() { };
};

HTTPD *httpd = NULL;

class ArgsHTTPD : public HTTPDhook {
public:

  string reply(const map<string, string> &params) {
    return "args args args";
  }

  ArgsHTTPD() : HTTPDhook("args") { };
} argshttpd;

class ParamsHTTPD : public HTTPDhook {
public:

  string reply(const map<string, string> &params) {
    string rv;
    for(map<string, HTTPDhook *>::const_iterator itr = getHooks().begin(); itr != getHooks().end(); itr++)
      rv += StringPrintf("<a href=\"%s\">/%s</a><br>", itr->first.c_str(), itr->first.c_str());
    return rv;
  }

  ParamsHTTPD() : HTTPDhook("") { };
} paramshttpd;

DEFINE_int(httpd_port, -1, "Port for HTTPD control interface (-1 to disable)");

void initHttpd() {
  CHECK(!httpd);
  if(FLAGS_httpd_port != -1)
    httpd = new HTTPD(FLAGS_httpd_port);
}

void tickHttpd() {
  StackString sst("httpdtick");
  if(httpd)
    httpd->process();
}

void deinitHttpd() {
  delete httpd;
  httpd = NULL;
}
