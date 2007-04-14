
#include "merger_weapon.h"
#include "merger_util.h"
#include "itemdb.h"
#include "parse.h"

#include <map>
#include <fstream>
#include <set>

using namespace std;

struct Weapondat {
  string cost;
  string firerate;
  
  string dpp;
};

void mergeWeapon(const string &csv, const string &unmerged, const string &merged) {
  map<string, Weapondat> weapondats;
  {
    ifstream ifs(csv.c_str());
    string lin;
    
    string prefix;
    int lastid = -1;
    
    bool gaci = true;
    
    while(getline(ifs, lin)) {
      CHECK(prefix.size() || lastid == -1);
      vector<string> dt = parseCsv(lin);
      
      if(dt[0] == "---END---")
        break;
      
      bool pline = false;
      
      if(dt[0] == "" || dt[0] == "WEAPON") {
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
        
        if(prefix == "Autocannon")
          gaci = false;
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
    ifstream ifs(unmerged.c_str());
    ofstream ofs(merged.c_str());
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
      } else if(kvd.category == "warhead") {
        dprintf("Warhead %s found\n", kvd.read("name").c_str());
        
        CHECK(count(kvd.read("name").begin(), kvd.read("name").end(), '.'));
        string basicname = strrchr(kvd.read("name").c_str(), '.') + 1;
        
        dprintf("%s\n", basicname.c_str());
        CHECK(basicname.size());
        basicname += ' ';
        for(int i = basicname.size() - 2; i >= 0; i--)
          if(basicname[i] == 'I' || basicname[i] == 'V')
            swap(basicname[i], basicname[i + 1]);
          else
            break;
        dprintf("%s\n", basicname.c_str());
        
        if(weapondats.count(basicname)) {
          CHECK(!doneweps.count(basicname));
          vector<string> radidam = tokenize(kvd.read("radiusdamage"), "\n ");
          CHECK(radidam.size() == 2);
          CHECK(radidam[0] == "MERGE");
          kvd.kv["radiusdamage"] = StringPrintf("%s %s", weapondats[basicname].dpp.c_str(), radidam[1].c_str());
        }
      }
      
      for(map<string, string>::const_iterator itr = kvd.kv.begin(); itr != kvd.kv.end(); itr++)
        CHECK(itr->second != "MERGE");
      
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
  addItemFile(merged);
  
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