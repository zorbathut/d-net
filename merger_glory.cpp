
#include "merger_glory.h"
#include "merger_util.h"
#include "debug.h"
#include "parse.h"
#include "itemdb.h"

#include <fstream>
#include <set>


string GloryParams::token() {
  return "GLORY";
}

bool GloryParams::parseLine(const vector<string> &line, Data *data) {
  data->cost = line[1];
  data->intended_damage = atof(line[2].c_str());
  return true;
}

string GloryParams::getWantedName(const string &name, const set<string> &possiblenames) {
  dprintf("GWN %s\n", name.c_str());
  string rv;
  rv = suffix(name);
  if(possiblenames.count(rv)) {
    dprintf("GWA %s\n", rv.c_str());
    return rv;
  }
  rv = findName(suffix(name, 2), possiblenames);
  if(possiblenames.count(rv)) {
    dprintf("GWB %s\n", rv.c_str());
    return rv;
  }
  dprintf("GWC\n");
  return "";
}

void GloryParams::preprocess(kvData *kvd, const Data &data) {
  if(kvd->category == "glory") {
    CHECK(kvd->read("cost") == "MERGE");
    
    kvd->kv["cost"] = data.cost;
  }
}

void GloryParams::testprocess(kvData *kvd, const Data &data) {
  if(kvd->category == "warhead") {
    if(kvd->kv.count("radiusdamage"))
      kvd->kv["radiusdamage"] = splice(kvd->read("radiusdamage"), 1);
    if(kvd->kv.count("impactdamage"))
      kvd->kv["impactdamage"] = splice(kvd->read("impactdamage"), 1);
  }
}

float GloryParams::getMultiple(const IDBGlory &item, const Data &data) {
  return data.intended_damage / IDBGloryAdjust(&item, IDBAdjustment()).stats_averageDamage();
}

void GloryParams::reprocess(kvData *kvd, const Data &data, float multiple) {
  if(kvd->category == "warhead") {
    if(kvd->kv.count("radiusdamage"))
      kvd->kv["radiusdamage"] = splice(kvd->read("radiusdamage"), multiple);
    if(kvd->kv.count("impactdamage"))
      kvd->kv["impactdamage"] = splice(kvd->read("impactdamage"), multiple);
  }
}

bool GloryParams::verify(const IDBGlory &item, const Data &data) {
  CHECK(withinEpsilon(IDBGloryAdjust(&item, IDBAdjustment()).stats_averageDamage(), data.intended_damage, 0.0001));
  return true;
}

const map<string, GloryParams::FinalType> &GloryParams::finalTypeList() { return gloryList(); }

/*
struct Glorydat {
  string cost;
  double intended_damage;
  
  float damage_unit;
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
      bd.cost = dt[1];
      bd.intended_damage = atof(dt[2].c_str());
    
      tdd[dt[0]] = bd;
    }
  }
  
  dprintf("Got %d glories\n", tdd.size());
  
  vector<kvData> okvds;
  {
    ifstream ifs(unmerged.c_str());
    kvData kvd;
    while(getkvData(ifs, &kvd)) {
      okvds.push_back(kvd);
    }
  }
  
  set<string> donetdd;
  {
    ofstream ofs(merged.c_str());
    for(int i = 0; i < okvds.size(); i++) {
      kvData kvd = okvds[i];
    
      if(kvd.category == "glory") {
        dprintf("Glory %s found\n", kvd.read("name").c_str());
        
        string basicname = suffix(kvd.read("name").c_str());
        
        CHECK(tdd.count(basicname));
        CHECK(!donetdd.count(basicname));
        
        CHECK(kvd.read("cost") == "MERGE");
        
        donetdd.insert(basicname);
        kvd.kv["cost"] = tdd[basicname].cost;
        
        okvds[i] = kvd;
      } else if(kvd.category == "warhead") {
        if(kvd.kv.count("radiusdamage"))
          kvd.kv["radiusdamage"] = splice(kvd.read("radiusdamage"), 1);
        if(kvd.kv.count("impactdamage"))
          kvd.kv["impactdamage"] = splice(kvd.read("impactdamage"), 1);
      }
      
      checkForExtraMerges(kvd);
      
      ofs << stringFromKvData(kvd) << endl;
    }
      
    if(donetdd.size() != tdd.size()) {
      dprintf("Glories incomplete!\n");
      for(map<string, Glorydat>::const_iterator itr = tdd.begin(); itr != tdd.end(); itr++)
        if(!donetdd.count(itr->first))
          dprintf("Didn't find %s\n", itr->first.c_str());
      CHECK(0);
    }
  }
  
  dprintf("bahm\n");
  
  // Now we load the files in and see how much data our weapons are actually doing.
  addItemFile("data/base/common.dwh");
  addItemFile("data/base/hierarchy.dwh");
  addItemFile(merged);
  
  for(map<string, Glorydat>::iterator itr = tdd.begin(); itr != tdd.end(); itr++) {
    dprintf("Parsing %s\n", itr->first.c_str());
    CHECK(gloryList().count("ROOT.Glory Devices." + itr->first));
    const float realdamage = IDBGloryAdjust(&gloryList().find("ROOT.Glory Devices." + itr->first)->second, IDBAdjustment()).stats_averageDamage();
    dprintf("  Does damage %f, intends damage %f\n", realdamage, itr->second.intended_damage);
    itr->second.damage_unit = itr->second.intended_damage / realdamage;
    dprintf("  Adjustment is %f\n", itr->second.damage_unit);
  }
  
  {
    ofstream ofs(merged.c_str());
    for(int i = 0; i < okvds.size(); i++) {
      kvData kvd = okvds[i];
    
      if(kvd.category == "warhead") {
        string tokname = findName(suffix(kvd.read("name"), 2), tdd);
        CHECK(tokname.size());
        
        if(kvd.kv.count("radiusdamage"))
          kvd.kv["radiusdamage"] = splice(kvd.read("radiusdamage"), tdd[tokname].damage_unit);
        if(kvd.kv.count("impactdamage"))
          kvd.kv["impactdamage"] = splice(kvd.read("impactdamage"), tdd[tokname].damage_unit);
      }
      
      checkForExtraMerges(kvd);
      
      ofs << stringFromKvData(kvd) << endl;
    }
  }
  
  clearItemdb();
  addItemFile("data/base/common.dwh");
  addItemFile("data/base/hierarchy.dwh");
  addItemFile(merged);
  
  bool shitisbroke = false;
  
  for(map<string, IDBGlory>::const_iterator itr = gloryList().begin(); itr != gloryList().end(); itr++) {
    CHECK(count(itr->first.begin(), itr->first.end(), '.'));
    string wname = strrchr(itr->first.c_str(), '.') + 1;
    CHECK(donetdd.count(wname)); // what
    donetdd.erase(wname);
    float dps = IDBGloryAdjust(&itr->second, IDBAdjustment()).stats_averageDamage();
    float ddps = tdd[suffix(itr->first)].intended_damage;
    if(abs(dps / ddps - 1.0) > 0.001) {
      dprintf("Glory %s doesn't do the right damage per shot (is %f, should be %f)\n", wname.c_str(), dps, ddps);
      shitisbroke = true;
    }
  }
  
  CHECK(!shitisbroke);
  
}
*/
