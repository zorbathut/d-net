
#include "httpd.h"
#include "itemdb.h"
using namespace std;

// shut up >:(

class ReloadHTTPD : public HTTPDhook {
public:

  string reply(const map<string, string> &params) {
    reloadItemdb();
    return "done";
  }

  ReloadHTTPD() : HTTPDhook("reload") { };
} reloadhttpd;
