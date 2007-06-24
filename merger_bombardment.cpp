
#include "merger_bombardment.h"
#include "merger_util.h"
#include "debug.h"
#include "parse.h"
#include "itemdb.h"

#include <fstream>
#include <set>

using namespace std;


string BombardParams::token() {
  return "BOMBARDMENT";
}

bool BombardParams::parseLine(const vector<string> &line, Data *data) {
  data->cost = line[1];
  data->dpp = line[3];
  data->lock = line[4];
  data->unlock = line[5];
  data->durability = line[6];
  return true;
}

string BombardParams::nameFromKvname(const string &name, const set<string> &possiblenames) {
  if(possiblenames.count(suffix(name)))
    return suffix(name);
  return findName(suffix(name), possiblenames);
}

void BombardParams::preprocess(kvData *kvd, const Data &data) {
  if(kvd->category == "bombardment") {
    CHECK(kvd->read("cost") == "MERGE");
    CHECK(kvd->read("lockdelay") == "MERGE");
    CHECK(kvd->read("unlockdelay") == "MERGE");
    
    kvd->kv["cost"] = data.cost;
    kvd->kv["lockdelay"] = data.lock;
    kvd->kv["unlockdelay"] = data.unlock;
  } else if(kvd->category == "projectile") {
    if(kvd->kv.count("durability") && kvd->kv["durability"] == "MERGE")
      kvd->kv["durability"] = data.durability;
  } else if(kvd->category == "warhead") {
    CHECK(kvd->kv.count("radiusdamage"));
    kvd->kv["radiusdamage"] = splice(kvd->read("radiusdamage"), data.dpp);
  }
}

bool BombardParams::verify(const IDBBombardment &item, const Data &data) {
  CHECK(withinEpsilon(item.cost.toFloat(), moneyFromString(data.cost).toFloat(), 0.0001));
  CHECK(withinEpsilon(item.lockdelay, atof(data.lock.c_str()), 0.0001));
  CHECK(withinEpsilon(item.unlockdelay, atof(data.unlock.c_str()), 0.0001));
  CHECK(withinEpsilon(IDBBombardmentAdjust(&item, IDBAdjustment(), 0).stats_damagePerShot(), atof(data.dpp.c_str()), 0.0001));
  return true;
}

const map<string, BombardParams::FinalType> &BombardParams::finalTypeList() { return bombardmentList(); }
