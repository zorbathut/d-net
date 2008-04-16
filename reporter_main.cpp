
#include <curl/curl.h>

#include "debug.h"

int main() {
  CHECK(curl_global_init(CURL_GLOBAL_ALL) == 0);
  
  dprintf("initted\n");
};
