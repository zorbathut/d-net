
#include "merger_upgrades.h"
#include "merger_util.h"
#include "debug.h"
#include "parse.h"
#include "itemdb.h"

#include <fstream>
#include <set>

string UpgradeParams::Namer::getName(const vector<string> &line) {
  if(line[0] == "")
    return "";
  return line[0] + "+ROOT.Tank Upgrades." + line[1];
}

string UpgradeParams::token() {
  return "UPGRADES";
}

bool UpgradeParams::parseLine(const vector<string> &line, Data *data) {
  data->costmult = line[3];
  return true;
}

string UpgradeParams::nameFromKvd(const kvData &kvd, const set<string> &possiblenames) {
  if(kvd.category != "upgrade")
    return "";
  string tname = kvd.read("category") + "+" + kvd.read("name");
  if(possiblenames.count(tname))
    return tname;
  return "";
}

void UpgradeParams::preprocess(kvData *kvd, const Data &data) {
  if(kvd->category == "upgrade") {
    CHECK(kvd->read("costmult") == "MERGE");
    
    kvd->kv["costmult"] = data.costmult;
  }
}

bool UpgradeParams::verify(const IDBUpgrade &item, const Data &data) {
  CHECK(withinEpsilon(item.costmult, atof(data.costmult.c_str()), 0.0001));
  return true;
}

const map<string, UpgradeParams::FinalType> &UpgradeParams::finalTypeList() { return upgradeList(); }
