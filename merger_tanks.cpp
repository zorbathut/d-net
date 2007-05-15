
#include "merger_tanks.h"
#include "merger_util.h"
#include "debug.h"
#include "parse.h"
#include "itemdb.h"

#include <fstream>
#include <set>

string TankParams::token() {
  return "TANKS";
}

bool TankParams::parseLine(const vector<string> &line, Data *data) {
  if(tokenize(line[0], " ")[0] == "Std")
    return false;
  data->health = line[2];
  data->engine= line[3];
  data->handling = line[4];
  data->mass = line[5];
  return true;
}

string TankParams::getWantedName(const string &name, const set<string> &possiblenames) {
  if(possiblenames.count(suffix(name)))
    return suffix(name);
  return "";
}

void TankParams::preprocess(kvData *kvd, const Data &data) {
  if(kvd->category == "tank") {
    CHECK(kvd->read("health") == "MERGE");
    CHECK(kvd->read("engine") == "MERGE");
    CHECK(kvd->read("handling") == "MERGE");
    CHECK(kvd->read("mass") == "MERGE");
    
    kvd->kv["health"] = data.health;
    kvd->kv["engine"] = data.engine;
    kvd->kv["handling"] = data.handling;
    kvd->kv["mass"] = data.mass;
  }
}

bool TankParams::verify(const IDBTank &item, const Data &data) {
  CHECK(withinEpsilon(item.health, atof(data.health.c_str()), 0.0001));
  CHECK(withinEpsilon(item.handling, atof(data.handling.c_str()), 0.0001));
  CHECK(withinEpsilon(item.engine, atof(data.engine.c_str()), 0.0001));
  CHECK(withinEpsilon(item.mass, atof(data.mass.c_str()), 0.0001));
  return true;
}

const map<string, TankParams::FinalType> &TankParams::finalTypeList() { return tankList(); }
