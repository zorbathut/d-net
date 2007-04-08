
#include "debug.h"
#include "parse.h"
#include "util.h"
#include "os.h"
#include "itemdb.h"
#include "args.h"

#include <vector>
#include <string>
#include <fstream>
#include <map>
#include <set>
#include <iostream>

using namespace std;

vector<string> parseCsv(const string &in) {
  dprintf("%s\n", in.c_str());
  vector<string> tok;
  {
    string::const_iterator bg = in.begin();
    while(bg != in.end()) {
      string::const_iterator nd = find(bg, in.end(), ',');
      tok.push_back(string(bg, nd));
      bg = nd;
      if(bg != in.end())
        bg++;
    }
  }
  for(int i = 0; i < tok.size(); i++) {
    if(tok[i].size() >= 2 && tok[i][0] == '"' && tok[i][tok[i].size() - 1] == '"') {
      tok[i].erase(tok[i].begin());
      tok[i].erase(tok[i].end() - 1);
    }
  }
  return tok;
}

struct Weapondat {
  string cost;
  string firerate;
  
  string dpp;
};

int main(int argc, char *argv[]) {
  set_exename("merger.exe");
  initFlags(argc, argv, 3);
  
  CHECK(argc == 4);
  
  map<string, Weapondat> weapondats;
  {
    ifstream ifs(argv[2]);
    string lin;
    
    string prefix;
    int lastid = -1;
    
    bool gaci = false;
    
    while(getline(ifs, lin)) {
      CHECK(prefix.size() || lastid == -1);
      vector<string> dt = parseCsv(lin);
      
      if(dt[0] == "---END---")
        break;
      
      bool pline = false;
      
      if(dt[0] == "") {
        CHECK(prefix.size() || lastid == -1);
        if(prefix.size() && dt[1] != "" && dt[1] != "Params") {
          CHECK(dt[1] == roman_number(lastid));
          lastid++;
          pline = true;
        }
      } else {
        CHECK(lastid == -1 || lastid == 6);
        prefix = dt[0];
        lastid = 0;
      }
      
      if(pline) {
        string name = prefix + " " + roman_number(lastid - 1);
        if(name == "Autocannon I") {
          CHECK(!gaci);
          gaci = true;
          CHECK(dt[2] == "100");
          CHECK(dt[3] == "5");
          CHECK(dt[5] == "1");  // if these are not true something bizarre has changed and we cannot trust our data
        }
        CHECK(!weapondats.count(name));
        
        Weapondat wd;
        wd.cost = dt[2];
        wd.firerate = dt[3];
        wd.dpp = dt[5];
        weapondats[name] = wd;
      }
    }
    CHECK(gaci);
  }
  
  dprintf("Got %d weapondats\n", weapondats.size());
  
  set<string> doneweps;
  {
    ifstream ifs(argv[1]);
    ofstream ofs(argv[3]);
    kvData kvd;
    while(getkvData(ifs, &kvd)) {
      if(kvd.category == "weapon") {
        dprintf("Weapon %s found\n", kvd.read("name").c_str());
        
        CHECK(count(kvd.read("name").begin(), kvd.read("name").end(), '.'));
        string basicname = strrchr(kvd.read("name").c_str(), '.') + 1;
        
        CHECK(weapondats.count(basicname));
        CHECK(!doneweps.count(basicname));
        
        CHECK(kvd.read("cost") == "MERGE");
        CHECK(kvd.read("firerate") == "MERGE");
        
        doneweps.insert(basicname);
        kvd.kv["cost"] = weapondats[basicname].cost;
        kvd.kv["firerate"] = weapondats[basicname].firerate;
      }
      
      ofs << stringFromKvData(kvd) << endl;
    }
    
    if(doneweps.size() != weapondats.size()) {
      dprintf("Weapons incomplete!\n");
      for(map<string, Weapondat>::const_iterator itr = weapondats.begin(); itr != weapondats.end(); itr++)
        if(!doneweps.count(itr->first))
          dprintf("Didn't complete %s\n", itr->first.c_str());
    }
  }
  
  addItemFile("data/base/common.dwh");
  addItemFile("data/base/hierarchy.dwh");
  addItemFile(argv[3]);
  
  bool shitisbroke = false;
  
  for(map<string, IDBWeapon>::const_iterator itr = weaponList().begin(); itr != weaponList().end(); itr++) {
    CHECK(count(itr->first.begin(), itr->first.end(), '.'));
    string wname = strrchr(itr->first.c_str(), '.') + 1;
    CHECK(doneweps.count(wname)); // what
    doneweps.erase(wname);
    float dps = IDBWeaponAdjust(&itr->second, IDBAdjustment()).launcher().stats_damagePerShot();
    float ddps = atof(weapondats[wname].dpp.c_str());
    if(dps != ddps) {
      dprintf("Weapon %s doesn't do the right damage per shot (is %f, should be %f)\n", wname.c_str(), dps, ddps);
      shitisbroke = true;
    }
  }
  CHECK(!shitisbroke);
}
