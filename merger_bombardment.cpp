
#include "merger_bombardment.h"
#include "debug.h"
#include "parse.h"

#include <fstream>

using namespace std;

void mergeBombardment(const string &csv, const string &unmerged, const string &merged) {
  {
    ifstream ifs(unmerged.c_str());
    ofstream ofs(merged.c_str());
    kvData kvd;
    while(getkvData(ifs, &kvd)) {
      for(map<string, string>::const_iterator itr = kvd.kv.begin(); itr != kvd.kv.end(); itr++)
        CHECK(itr->second != "MERGE");
      
      ofs << stringFromKvData(kvd) << endl;
    }
  }
}
