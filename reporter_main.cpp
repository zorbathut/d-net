
#include <curl/curl.h>
#include <map>
#include <zlib.h>

#include "debug.h"
#include "util.h"

using namespace std;

size_t writefunc(void *ptr, size_t size, size_t nmemb, void *lulz) {
  dprintf("appendinating %d\n", size * nmemb);
  string *odat = (string*)lulz;
  *odat += string((char*)ptr, (char*)ptr + size * nmemb);
  return size * nmemb;
}

string cee(CURL *curl, const string &str) {
  char *escy = curl_easy_escape(curl, str.c_str(), str.size());
  CHECK(escy);
  CHECK(strlen(escy) >= str.size());
  string rv = escy;
  curl_free(escy);
  return rv;
}

string request(string url, const map<string, string> &posts) {
  CURL *handle = curl_easy_init();
  CHECK(handle);
  
  char errbuf[CURL_ERROR_SIZE];
  string out;
  
  curl_easy_setopt(handle, CURLOPT_ERRORBUFFER, errbuf);
  curl_easy_setopt(handle, CURLOPT_WRITEFUNCTION, writefunc);
  curl_easy_setopt(handle, CURLOPT_WRITEDATA, &out);
  
  string urlencode;
  for(map<string, string>::const_iterator itr = posts.begin(); itr != posts.end(); itr++) {
    if(urlencode.size())
      urlencode += "&";
    urlencode += cee(handle, itr->first) + "=" + cee(handle, itr->second);
  }
  dprintf("urlencode is %s\n", urlencode.c_str());
  
  curl_easy_setopt(handle, CURLOPT_POSTFIELDS, urlencode.c_str());
  curl_easy_setopt(handle, CURLOPT_URL, url.c_str());
  
  if(curl_easy_perform(handle)) {
    dprintf("Error: %s\n", errbuf);
    CHECK(0);
  }
  
  curl_easy_cleanup(handle);
  
  return out;
}

int main(int argc, const char *argv[]) {
  CHECK(argc == 6);
  
  dprintf("REPORTER IS YOUR FREND\n");
  
  // display message box here
  
  CHECK(curl_global_init(CURL_GLOBAL_ALL) == 0);
  
  string url = StringPrintf("http://crashlog.cams.local/%s.php", argv[1]);
  
  map<string, string> parms;
  string res;
  parms["instruction"] = "request";
  parms["file"] = argv[3];
  parms["line"] = argv[4];
  parms["exesize"] = argv[5];
  
  res = request(url, parms);
  
  if(res != "OK") {
    // display message box here
    return 1;
  }
  
  string krlog;
  {
    z_stream comp;
    comp.zalloc = Z_NULL;
    comp.zfree = Z_NULL;
    comp.opaque = 0;
    CHECK(deflateInit2(&comp, 5, Z_DEFLATED, 15, 8, Z_DEFAULT_STRATEGY) == Z_OK);
    
    FILE *fil = fopen(argv[2], "rb");
    CHECK(fil);
    char buff[65536];
    char out[65536];
    comp.next_out = (Byte*)out;
    comp.avail_out = sizeof(out);
    int dat;
    while((dat = fread(buff, 1, sizeof(buff), fil)) > 0) {
      comp.next_in = (Byte*)buff;
      comp.avail_in = dat;
      while(comp.avail_in) {
        CHECK(deflate(&comp, 0) == Z_OK);
        krlog += string(out, out + sizeof(out) - comp.avail_out);
        dprintf("%d, %d, %d\n", krlog.size(), comp.total_out, comp.avail_out);
        CHECK(krlog.size() == comp.total_out);
        comp.next_out = (Byte*)out;
        comp.avail_out = sizeof(out);
      }
    }
    
    while(1) {
      int rv = deflate(&comp, Z_FINISH);
      dprintf("%d\n", rv);
      CHECK(rv == Z_OK || rv == Z_STREAM_END);
      krlog += string(out, out + sizeof(out) - comp.avail_out);
      dprintf("%d, %d\n", krlog.size(), comp.total_out);
      CHECK(krlog.size() == comp.total_out);
      comp.next_out = (Byte*)out;
      comp.avail_out = sizeof(out);
      if(rv == Z_STREAM_END)
        break;
    }
    
    fclose(fil);
    
    dprintf("compressed %d to %d\n", (int)comp.total_in, (int)comp.total_out);
    CHECK(krlog.size() == comp.total_out);
    
    CHECK(deflateEnd(&comp) == Z_OK);
  }
  
  parms["dump"] = krlog;
  parms["instruction"] = "dump";

  res = request(url, parms);
  
  dprintf("res is %s\n", res.c_str());
  
  // display message box here
  dprintf("dun!\n");
  
};
