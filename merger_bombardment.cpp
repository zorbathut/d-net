
#include "merger_bombardment.h"
#include "merger_util.h"
#include "debug.h"
#include "parse.h"
#include "itemdb.h"

#include <fstream>
#include <set>

using namespace std;

struct Bombarddat {
  string cost;
  
  string dpp;
  
  string lock;
  string unlock;
};

void mergeBombardment(const string &csv, const string &unmerged, const string &merged) {
  map<string, Bombarddat> bdd;
  {
    ifstream ifs(csv.c_str());
    
    string lin;
    while(getline(ifs, lin)) {
      vector<string> dt = parseCsv(lin);
      
      if(dt[0] == "BOMBARDMENT" || dt[0] == "")
        continue;
      
      CHECK(!bdd.count(dt[0]));
      
      Bombarddat bd;
      bd.cost = dt[1];
      bd.dpp = dt[3];
      bd.lock = dt[4];
      bd.unlock = dt[5];
    
      bdd[dt[0]] = bd;
    }
  }
  
  dprintf("Got %d bombards\n", bdd.size());
  
  set<string> donebdd;
  {
    ifstream ifs(unmerged.c_str());
    ofstream ofs(merged.c_str());
    kvData kvd;
    while(getkvData(ifs, &kvd)) {
      if(kvd.category == "bombardment") {
        dprintf("Bombardment %s found\n", kvd.read("name").c_str());
        
        string basicname = suffix(kvd.read("name").c_str());
        
        CHECK(bdd.count(basicname));
        CHECK(!donebdd.count(basicname));
        
        CHECK(kvd.read("cost") == "MERGE");
        CHECK(kvd.read("lockdelay") == "MERGE");
        CHECK(kvd.read("unlockdelay") == "MERGE");
        
        donebdd.insert(basicname);
        kvd.kv["cost"] = bdd[basicname].cost;
        kvd.kv["lockdelay"] = bdd[basicname].lock;
        kvd.kv["unlockdelay"] = bdd[basicname].unlock;
      } else if(kvd.category == "warhead") {
        dprintf("Warhead %s found\n", kvd.read("name").c_str());
        
        string matchname = findName(suffix(kvd.read("name")), bdd);
        
        if(matchname.size())
          kvd.kv["radiusdamage"] = splice(kvd.read("radiusdamage"), bdd[matchname].dpp);
      }
      
      for(map<string, string>::const_iterator itr = kvd.kv.begin(); itr != kvd.kv.end(); itr++)
        if(itr->second.find("MERGE") != string::npos)
          dprintf("%s\n", stringFromKvData(kvd).c_str());
      
      ofs << stringFromKvData(kvd) << endl;
    }
    
    if(donebdd.size() != bdd.size()) {
      dprintf("Bombards incomplete!\n");
      for(map<string, Bombarddat>::const_iterator itr = bdd.begin(); itr != bdd.end(); itr++)
        if(!donebdd.count(itr->first))
          dprintf("Didn't complete %s\n", itr->first.c_str());
      //CHECK(0);
    }
  }
  
  // this is really just for parse testing atm
  addItemFile("data/base/common.dwh");
  addItemFile("data/base/hierarchy.dwh");
  addItemFile(merged);
}
