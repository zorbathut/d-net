
#include "httpd.h"

#include "args.h"
#include "debug.h"
#include "parse.h"
#include "socket.h"
#include "util.h"

using namespace std;

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
      
      bool valid = true;
      
      map<string, string> params;
      if(count(location.begin(), location.end(), '?')) {
        dprintf("Snagerating params\n");
        vector<string> tok = tokenize(strchr(location.c_str(), '?') + 1, "&");
        for(int i = 0; i < tok.size(); i++) {
          vector<string> toki = tokenize(tok[i], "=");
          if(toki.size() != 2 || params.count(toki[0])) {
            valid = false;
            break;
          }
          params[toki[0]] = toki[1];
          dprintf("Param %s set to %s\n", toki[0].c_str(), toki[1].c_str());
        }
        location = string(location.c_str(), (const char*)strchr(location.c_str(), '?'));
      }
      
      if(!getHooks().count(location) || !valid) {
        ptr->sendline("HTTP/1.1 404 File Not Found");
        ptr->sendline("Content-type: text/html");
        ptr->sendline("");
        ptr->sendline(StringPrintf("404 for /%s:<p>", location.c_str()));
        if(!valid)
          ptr->sendline("error'd");
        else
          ptr->sendline("owned");
      } else {
        ptr->sendline("HTTP/1.1 200 OK");
        ptr->sendline("Content-type: text/html");
        ptr->sendline("");
        ptr->sendline(StringPrintf("Listing for /%s:<p>", location.c_str()));
        ptr->sendline(getHooks()[location]->reply(params));
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
