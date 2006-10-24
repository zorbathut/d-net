
#include "httpd.h"

#include "socket.h"
#include "debug.h"
#include "args.h"
#include "parse.h"
#include "util.h"

map<string, HTTPDhook *> hooks;

HTTPDhook::HTTPDhook(const string &identifier) : identifier(identifier) {
  CHECK(!hooks.count(identifier));
  hooks[identifier] = this;
};
HTTPDhook::~HTTPDhook() {
  CHECK(hooks.count(identifier));
  CHECK(hooks[identifier] == this);
  hooks.erase(identifier);
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
      
      if(!hooks.count(location)) {
        ptr->sendline("HTTP/1.1 404 File Not Found");
        ptr->sendline("Content-type: text/html");
        ptr->sendline("");
        ptr->sendline("owned");
      } else {
        ptr->sendline("HTTP/1.1 200 OK");
        ptr->sendline("Content-type: text/html");
        ptr->sendline("");
        ptr->sendline(StringPrintf("Listing for /%s:<p>", location.c_str()));
        ptr->sendline(hooks[location]->reply(map<string, string>()));
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
};

ArgsHTTPD *httpdargs = NULL;

class ParamsHTTPD : public HTTPDhook {
public:

  string reply(const map<string, string> &params) {
    string rv;
    for(map<string, HTTPDhook *>::const_iterator itr = hooks.begin(); itr != hooks.end(); itr++)
      rv += StringPrintf("<a href=\"%s\">/%s</a><br>", itr->first.c_str(), itr->first.c_str());
    return rv;
  }

  ParamsHTTPD() : HTTPDhook("") { };
};

ParamsHTTPD *httpdparams = NULL;

DEFINE_int(httpd_port, -1, "Port for HTTPD control interface (-1 to disable)");

void initHttpd() {
  CHECK(!httpd);
  CHECK(!httpdargs);
  CHECK(!httpdparams);
  if(FLAGS_httpd_port != -1) {
    httpd = new HTTPD(FLAGS_httpd_port);
    httpdargs = new ArgsHTTPD;
    httpdparams = new ParamsHTTPD;
  }
}

void tickHttpd() {
  StackString sst("httpdtick");
  if(httpd)
    httpd->process();
}

void deinitHttpd() {
  delete httpdparams;
  delete httpdargs;
  delete httpd;
  httpdparams = NULL;
  httpdargs = NULL;
  httpd = NULL;
}
