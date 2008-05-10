
#include <curl/curl.h>

#include "debug.h"

int main(int argc, const char *argv[]) {
  dprintf("REPORTER IS YOUR FREND\n");
  
  CHECK(curl_global_init(CURL_GLOBAL_ALL) == 0);
  
  CURL *handle = curl_easy_init();
  CHECK(handle);
  
  char errbuf[CURL_ERROR_SIZE];
  curl_easy_setopt(handle, CURLOPT_ERRORBUFFER, errbuf);
  
  curl_easy_setopt(handle, CURLOPT_URL, "http://crashlog.cams.local/report.php");
  dprintf("setopt\n");
  if(curl_easy_perform(handle)) {
    dprintf("Error: %s\n", errbuf);
  }
  
  
  curl_easy_cleanup(handle);
};
