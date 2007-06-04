
#include "merger_weapon.h"
#include "merger_util.h"
#include "itemdb.h"
#include "parse.h"

#include <map>
#include <fstream>
#include <set>

using namespace std;

string WeaponParams::Namer::getName(const vector<string> &line) {
  if(line[0] == "") {
    CHECK(prefix.size() || lastid == -1);
    if(prefix.size() && line[1] != "" && line[1] != "Params") {
      CHECK(line[1] == roman_number(lastid));
      lastid++;
      return prefix + " " + line[1];
    } else if(line[1] == "Params") {
      CHECK(prefix.size());
      return prefix;
    } else {
      return "";
    }
  } else {
    CHECK(lastid == -1 || lastid == 6);
    prefix = line[0];
    lastid = 0;
    return "";
  }
}

WeaponParams::Namer::Namer() {
  lastid = -1;
}

string WeaponParams::token() {
  return "WEAPON";
}

bool WeaponParams::parseLine(const vector<string> &line, Data *data) {
  if(line[1] == "Params") {
    CHECK(!line[6].size());
    CHECK(line[8].size());
    if(!line[7].size())
      return false;
    data->params = true;
    data->dpp = atof(line[7].c_str());
    data->durability = line[5];
    data->params_threshold = string(line[2].begin(), find(line[2].begin(), line[2].end(), '.'));
    CHECK(data->params_threshold.size());
    return true;
  } else {
    data->params = false;
    data->item_cost = line[2];
    data->item_firerate = line[3];
    CHECK(line[4].size());
    data->item_recommended = StringPrintf("%d", atoi(line[4].c_str()));
    data->dpp = atof(line[7].c_str());
    data->durability = line[5];
    return true;
  }
}

string WeaponParams::getWantedName(const string &name, const set<string> &possiblenames) {
  //dprintf("--- START %s\n", name.c_str());
  string rv;
  rv = suffix(name);
  //dprintf("%s\n", rv.c_str());
  if(possiblenames.count(rv))
    return rv;
  rv = findName(suffix(name, 1), possiblenames);
  //dprintf("%s\n", rv.c_str());
  if(possiblenames.count(rv))
    return rv;
  rv = findName(suffix(name, 2), possiblenames);
  //dprintf("%s\n", rv.c_str());
  if(possiblenames.count(rv))
    return rv;
  //dprintf("returning NOTHING\n");
  return "";
}

void WeaponParams::preprocess(kvData *kvd, const Data &data) {
  StackString prep(StringPrintf("Preprocessing %s", kvd->saferead("name").c_str()));
  if(kvd->category == "weapon") {
    CHECK(!data.params);
    
    CHECK(kvd->read("cost") == "MERGE");
    CHECK(kvd->read("firerate") == "MERGE");
    CHECK(!kvd->kv.count("recommended") || kvd->read("recommended") == "MERGE");
    
    kvd->kv["cost"] = data.item_cost;
    kvd->kv["firerate"] = data.item_firerate;
    kvd->kv["recommended"] = data.item_recommended;
  } else if(kvd->category == "hierarchy") {
    CHECK(data.params);
    
    CHECK(atof(data.params_threshold.c_str()) < 1000 || kvd->read("spawncash") == "MERGE");
    
    if(kvd->kv.count("spawncash"))
      kvd->kv["spawncash"] = data.params_threshold;
  } else if(kvd->category == "projectile") {
    if(kvd->kv.count("durability") && kvd->read("durability") == "MERGE")
      kvd->kv["durability"] = data.durability;
  }
}

void WeaponParams::testprocess(kvData *kvd) {
  if(kvd->category == "warhead") {
    if(kvd->kv.count("radiusdamage"))
      kvd->kv["radiusdamage"] = splice(kvd->read("radiusdamage"), 1);
    if(kvd->kv.count("impactdamage"))
      kvd->kv["impactdamage"] = splice(kvd->read("impactdamage"), 1);
  }
}

float WeaponParams::getMultiple(const IDBWeapon &item, const Data &data) {
  return data.dpp / IDBWeaponAdjust(&item, IDBAdjustment()).launcher().stats_damagePerShot();
}

string WeaponParams::getMultipleAltName(const string &name) {
  string suff = suffix(name);
  if(!count(suff.begin(), suff.end(), ' '))
    return "";
  {
    string pref = strrchr(suff.c_str(), ' ') + 1;
    bool found = false;
    for(int i = 0; i < 6; i++)
      if(pref == roman_number(i))
        found = true;
    if(!found)
      return "";
  }
  suff = string(suff.c_str(), (const char *)strrchr(suff.c_str(), ' '));
  return suff;
}

void WeaponParams::reprocess(kvData *kvd, float multiple) {
  if(kvd->category == "warhead") {
    if(kvd->kv.count("radiusdamage"))
      kvd->kv["radiusdamage"] = splice(kvd->read("radiusdamage"), multiple);
    if(kvd->kv.count("impactdamage"))
      kvd->kv["impactdamage"] = splice(kvd->read("impactdamage"), multiple);
  }
}

bool WeaponParams::verify(const IDBWeapon &item, const Data &data) {
  CHECK(withinEpsilon(IDBWeaponAdjust(&item, IDBAdjustment()).launcher().stats_damagePerShot(), data.dpp, 0.0001));
  if(data.durability.size())
    CHECK(withinEpsilon(IDBWeaponAdjust(&item, IDBAdjustment()).launcher().deploy().chain_projectile()[0].durability(), atof(data.durability.c_str()), 0.0001));
  return true;
}

const map<string, WeaponParams::FinalType> &WeaponParams::finalTypeList() { return weaponList(); }
