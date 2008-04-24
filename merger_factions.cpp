
#include "merger_factions.h"
#include "merger_util.h"
#include "debug.h"
#include "parse.h"
#include "itemdb.h"

#include <fstream>
#include <set>

string FactionParams::Namer::getName(const vector<string> &line) {
  if(!line[0].size())
    return "";
  string foo = tokenize(line[0], " '")[0];
  for(int i = 0; i < foo.size(); i++)
    foo[i] = tolower(foo[i]);
  return foo;
}

string FactionParams::token() {
  return "FACTIONS";
}

bool FactionParams::isDemoable(const Data &toki) {
  return true;
}

static const int darray[] = { IDBAdjustment::DAMAGE_KINETIC, IDBAdjustment::DAMAGE_ENERGY, IDBAdjustment::DAMAGE_EXPLOSIVE, IDBAdjustment::DAMAGE_TRAP, IDBAdjustment::DAMAGE_EXOTIC, IDBAdjustment::WARHEAD_RADIUS_FALLOFF, IDBAdjustment::DAMAGE_ALL, IDBAdjustment::TANK_FIRERATE, IDBAdjustment::TANK_SPEED, IDBAdjustment::TANK_TURN, IDBAdjustment::TANK_ARMOR, IDBAdjustment::DISCOUNT_WEAPON, IDBAdjustment::DISCOUNT_IMPLANT, IDBAdjustment::DISCOUNT_UPGRADE, IDBAdjustment::DISCOUNT_TANK, IDBAdjustment::RECYCLE_BONUS, IDBAdjustment::ALL };
  
bool FactionParams::parseLine(const vector<string> &line, Data *data) {
  for(int i = 0; i < ARRAY_SIZE(darray); i++) {
    if(line[i + 1].size()) {
      data->modifiers[adjust_text[darray[i]]] = line[i + 1];
    }
  }
  return true;
}

string FactionParams::nameFromKvname(const string &name, const set<string> &possiblenames) {
  if(possiblenames.count(suffix(name)))
    return suffix(name);
  return findName(suffix(name), possiblenames);
}

void FactionParams::preprocess(kvData *kvd, const Data &data) {
  if(kvd->category == "adjustment") {
    CHECK(kvd->kv.size() == 1);
    CHECK(kvd->kv.count("name"));
    
    for(map<string, string>::const_iterator cit = data.modifiers.begin(); cit != data.modifiers.end(); cit++)
      kvd->kv[cit->first] = cit->second;
  }
}

vector<string> FactionParams::dependencies() {
  return vector<string>();
}

bool FactionParams::verify(const IDBFaction &item, const Data &data) {
  // this is hard so we're not bothering
  return true;
}

static map<string, IDBFaction> dat;
const map<string, FactionParams::FinalType> &FactionParams::finalTypeList() {
  if(dat.size())
    return dat;
  Namer namer;
  for(int i = 0; i < factionList().size(); i++) {
    vector<string> nmv(1, factionList()[i].name);
    string tnm = namer.getName(nmv);
    dat[tnm] = factionList()[i];
  }
  CHECK(dat.size());
  return finalTypeList();
};
