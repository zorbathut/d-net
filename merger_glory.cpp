
#include "merger_glory.h"
#include "merger_util.h"
#include "debug.h"
#include "parse.h"
#include "itemdb.h"

#include <fstream>
#include <set>

struct Glorydat {
  string cost;
  double damage;
};

void mergeGlory(const string &csv, const string &unmerged, const string &merged) {
  map<string, Glorydat> tdd;
  {
    ifstream ifs(csv.c_str());
    
    string lin;
    while(getline(ifs, lin)) {
      vector<string> dt = parseCsv(lin);
      
      if(dt[0] == "GLORY" || dt[0] == "")
        continue;
      
      CHECK(!tdd.count(dt[0]));
      
      Glorydat bd;
      bd.cost = dt[2];
      bd.damage= atof(dt[3].c_str());
    
      tdd[dt[0]] = bd;
    }
  }
  
  dprintf("Got %d glories\n", tdd.size());
  
  set<string> donetdd;
  {
    ifstream ifs(unmerged.c_str());
    ofstream ofs(merged.c_str());
    kvData kvd;
    while(getkvData(ifs, &kvd)) {
      /*if(kvd.category == "tank") {
        dprintf("Tank %s found\n", kvd.read("name").c_str());
        
        string basicname = suffix(kvd.read("name").c_str());
        
        CHECK(tdd.count(basicname));
        CHECK(!donetdd.count(basicname));
        
        CHECK(kvd.read("health") == "MERGE");
        CHECK(kvd.read("engine") == "MERGE");
        CHECK(kvd.read("handling") == "MERGE");
        CHECK(kvd.read("mass") == "MERGE");
        
        donetdd.insert(basicname);
        kvd.kv["health"] = tdd[basicname].health;
        kvd.kv["engine"] = tdd[basicname].engine;
        kvd.kv["handling"] = tdd[basicname].handling;
        kvd.kv["mass"] = tdd[basicname].mass;
      }*/
      
      for(map<string, string>::const_iterator itr = kvd.kv.begin(); itr != kvd.kv.end(); itr++)
        CHECK(itr->second.find("MERGE") == string::npos);
      
      ofs << stringFromKvData(kvd) << endl;
    }
    
    if(donetdd.size() != tdd.size()) {
      dprintf("Glories incomplete!\n");
      for(map<string, Glorydat>::const_iterator itr = tdd.begin(); itr != tdd.end(); itr++)
        if(!donetdd.count(itr->first))
          dprintf("Didn't complete %s\n", itr->first.c_str());
      //CHECK(0);
    }
  }
  
  // this is really just for parse testing atm
  addItemFile("data/base/common.dwh");
  addItemFile("data/base/hierarchy.dwh");
  addItemFile(merged);
  
  // TODO: Check to make sure it does the right damage per shot!
}
