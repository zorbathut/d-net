
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

string GloryParams::nameFromKvname(const string &name, const set<string> &possiblenames) {
  string rv;
  rv = suffix(name);
  if(possiblenames.count(rv))
    return rv;
  rv = findName(suffix(name, 2), possiblenames);
  if(possiblenames.count(rv))
    return rv;
  return "";
}

void GloryParams::preprocess(kvData *kvd, const Data &data) {
  if(kvd->category == "glory") {
    CHECK(kvd->read("cost") == "MERGE");
    
    kvd->kv["cost"] = data.cost;
  }
}

void GloryParams::testprocess(kvData *kvd) {
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

string GloryParams::getMultipleAltName(const string &name) {
  return "";
}

void GloryParams::reprocess(kvData *kvd, float multiple) {
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
