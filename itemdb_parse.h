#ifndef DNET_ITEMDB_PARSE
#define DNET_ITEMDB_PARSE


#include <vector>
using namespace std;


void parseItemFile(const string &fname, bool reload, vector<string> *errors);
void parseShopcacheFile(const string &fname, vector<string> *errors);
  
#endif
