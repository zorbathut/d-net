
#include "itemdb.h"

#include "args.h"
#include "parse.h"
#include "httpd.h"

#include <fstream>
#include <numeric>

using namespace std;

static HierarchyNode root;
static map<string, IDBDeploy> deployclasses;
static map<string, IDBWarhead> warheadclasses;
static map<string, IDBProjectile> projectileclasses;
static map<string, IDBStats> statsclasses;
static map<string, IDBLauncher> launcherclasses;
static map<string, IDBWeapon> weaponclasses;
static map<string, IDBUpgrade> upgradeclasses;
static map<string, IDBGlory> gloryclasses;
static map<string, IDBBombardment> bombardmentclasses;
static map<string, IDBTank> tankclasses;
static map<string, IDBAdjustment> adjustmentclasses;
static map<string, IDBEffects> effectsclasses;
static map<string, IDBImplantSlot> implantslotclasses;
static map<string, IDBImplant> implantclasses;

static vector<IDBFaction> factions;
static map<string, string> text;

static const IDBTank *deftank = NULL;
static const IDBGlory *defglory = NULL;
static const IDBBombardment *defbombardment = NULL;

static map<string, IDBShopcache> shopcaches;

DEFINE_bool(debugitems, false, "Enable debug items");
DEFINE_bool(shopcache, true, "Enable shop demo cache");

//const char * const adjust_text[] = { "damage_kinetic", "damage_energy", "damage_explosive", "damage_trap", "damage_exotic", "warhead_radius_falloff", "discount_weapon", "discount_training", "discount_upgrade", "discount_tank", "recycle_bonus", "tank_firerate", "tank_speed", "tank_turn", "tank_armor", "damage_all", "all" };

string gendiffstring(int amount, const int (&arr)[11]) {
  for(int i = 0; i < ARRAY_SIZE(arr); i++) {
    if(amount <= arr[i]) {
      if(i == 5)
        return "~=";
      return string(abs(5 - i), amount < 5 ? '-' : '+');
    }
  }
  dprintf("%d\n", amount);
  CHECK(0);
}

const int weapon_diffs[11] = {-50, -30, -20, -10, -5, 5, 10, 20, 35, 50, 75};
const int discount_diffs[11] = {-50, -30, -20, -10, -5, 5, 10, 20, 35, 50, 75};
const int tank_diffs[11] = {-50, -30, -20, -10, -5, 5, 10, 20, 35, 50, 75};
const int warhead_diffs[11] = {-50, -30, -20, -10, -5, 5, 10, 20, 35, 50, 75};
const int recycle_diffs[11] = {-200, -100, -50, -25, -5, 5, 50, 100, 200, 400};
const int all_diffs[11] = {-50, -30, -20, -10, -1, 1, 10, 20, 35, 50, 75};

string adjust_modifiertext(int id, int amount) {
  if(id < IDBAdjustment::DAMAGE_LAST || id == IDBAdjustment::DAMAGE_ALL) {
    return gendiffstring(amount, weapon_diffs);
  } else if(id >= IDBAdjustment::DISCOUNT_BEGIN && id < IDBAdjustment::DISCOUNT_END) {
    return gendiffstring(amount, discount_diffs);
  } else if(id >= IDBAdjustment::TANK_BEGIN && id < IDBAdjustment::TANK_END) {
    return gendiffstring(amount, tank_diffs);
  } else if(id == IDBAdjustment::WARHEAD_RADIUS_FALLOFF) {
    return gendiffstring(amount, warhead_diffs);
  } else if(id == IDBAdjustment::ALL) {
    return gendiffstring(amount, all_diffs);
  } else if(id == IDBAdjustment::RECYCLE_BONUS) {
    return gendiffstring(amount, recycle_diffs);
  } else {
    CHECK(0);
  }
}

void IDBAdjustment::debugDump() const {
  dprintf("IDBAdjustment debug dump");
  for(int i = 0; i < LAST; i++)
    if(adjusts[i])
      dprintf("%12s: %d", adjust_text[i], adjusts[i]);
}

IDBAdjustment::IDBAdjustment() {
  memset(adjusts, 0, sizeof(adjusts));
  memset(adjustlist, -1, sizeof(adjustlist));
  ignore_excessive_radius = false;
}

IDBAdjustment operator*(const IDBAdjustment &lhs, int mult) {
  IDBAdjustment rv = lhs;
  for(int i = 0; i < IDBAdjustment::LAST; i++)
    rv.adjusts[i] *= mult;
  return rv;
}

const IDBAdjustment &operator+=(IDBAdjustment &lhs, const IDBAdjustment &rhs) {
  for(int i = 0; i < IDBAdjustment::LAST; i++)
    lhs.adjusts[i] += rhs.adjusts[i];
  return lhs;
}

bool operator==(const IDBAdjustment &lhs, const IDBAdjustment &rhs) {
  for(int i = 0; i < IDBAdjustment::LAST; i++)
    if(lhs.adjusts[i] != rhs.adjusts[i])
      return false;
  return true;
}

float IDBAdjustment::adjustmentfactor(int type) const {
  CHECK(type >= 0 && type < LAST);
  if(type == WARHEAD_RADIUS_FALLOFF) {
    if(!ignore_excessive_radius && adjusts[type] + 100 > WARHEAD_RADIUS_MAXMULT * 100) {
      dprintf("Adjust is %d vs %d\n", adjusts[type] + 100, WARHEAD_RADIUS_MAXMULT * 100);
      CHECK(adjusts[type] + 100 <= WARHEAD_RADIUS_MAXMULT * 100);
    }
  }
  return (float)(adjusts[type] + 100) / 100;
}

float IDBAdjustment::recyclevalue() const {
  float shares = 1 * adjustmentfactor(IDBAdjustment::RECYCLE_BONUS);
  float ratio = shares / (shares + 1.0);
  return ratio;
}

class ErrorAccumulator {
public:
  void addError(const string &text) {
    string tt = StringPrintf("%s:%d - %s", fname.c_str(), line, text.c_str());
    dprintf("%s\n", tt.c_str());
    errors->push_back(tt);
  }

  ErrorAccumulator(vector<string> *errors, const string &fname, int line) : errors(errors), fname(fname), line(line) { };
  
private:
  vector<string> *errors;
  string fname;
  int line;
};

void HierarchyNode::checkConsistency(vector<string> *errors) const {
  //dprintf("Consistency scan entering %s\n", name.c_str());
  // all nodes need a name
  CHECK(name.size());
  
  // check that type is within bounds
  CHECK(type >= 0 && type < HNT_LAST);
  
  // check that displaymode is within bounds
  CHECK(displaymode >= 0 && displaymode < HNDM_LAST);
  
  bool gottype = false;
  
  // categories don't have a cost and aren't buyable
  if(type == HNT_CATEGORY) {
    CHECK(!gottype);
    gottype = true;
    CHECK(displaymode != HNDM_COST);
    CHECK(!buyable);
  }
  
  // weapons all have costs and are buyable
  if(type == HNT_WEAPON) {
    CHECK(!gottype);
    gottype = true;
    CHECK(displaymode == HNDM_COST);
    CHECK(buyable);
    CHECK(weapon);
  } else {
    CHECK(!weapon);
  }
  
  // upgrades all are unique and are buyable
  if(type == HNT_UPGRADE) {
    CHECK(!gottype);
    gottype = true;
    CHECK(displaymode == HNDM_COSTUNIQUE);
    CHECK(buyable);
    CHECK(pack == 1);
    CHECK(upgrade);
  } else {
    CHECK(!upgrade);
  }
  
  // glory devices all are unique and are buyable
  if(type == HNT_GLORY) {
    CHECK(!gottype);
    gottype = true;
    CHECK(displaymode == HNDM_COSTUNIQUE);
    CHECK(pack == 1);
    CHECK(buyable);
    CHECK(glory);
  } else {
    CHECK(!glory);
  }
  
  // bombardment devices all are unique and are buyable
  if(type == HNT_BOMBARDMENT) {
    CHECK(!gottype);
    gottype = true;
    CHECK(displaymode == HNDM_COSTUNIQUE);
    CHECK(pack == 1);
    CHECK(buyable);
    CHECK(bombardment);
  } else {
    CHECK(!bombardment);
  }
  
  // tanks all are unique and are buyable
  if(type == HNT_TANK) {
    CHECK(!gottype);
    gottype = true;
    CHECK(displaymode == HNDM_COSTUNIQUE);
    CHECK(pack == 1);
    CHECK(buyable);
    CHECK(tank);
  } else {
    CHECK(!tank);
  }
  
  // the equip item restricts on EQUIPWEAPON but is not buyable
  if(type == HNT_EQUIP) {
    CHECK(!gottype);
    gottype = true;
    CHECK(displaymode == HNDM_BLANK);
    CHECK(!buyable);
    CHECK(cat_restrictiontype == HNT_EQUIPWEAPON);
  }
  
  // the sell item restricts on NONE and is not buyable
  if(type == HNT_SELL) {
    CHECK(!gottype);
    gottype = true;
    CHECK(displaymode == HNDM_BLANK);
    CHECK(!buyable);
    CHECK(cat_restrictiontype == HNT_NONE);
  }
  
  // the equipweapon item must have a weapon and is "buyable"
  if(type == HNT_EQUIPWEAPON) {
    CHECK(!gottype);
    gottype = true;
    CHECK(displaymode == HNDM_EQUIP);
    CHECK(buyable);
    CHECK(pack == 1);
    CHECK(equipweapon);
  } else {
    CHECK(!equipweapon);
  }
  
  // implantslot must be buyable and have a slot of 1
  if(type == HNT_IMPLANTSLOT) {  
    CHECK(!gottype);
    gottype = true;
    CHECK(displaymode == HNDM_COSTUNIQUE);
    CHECK(buyable);
    CHECK(pack == 1);
    CHECK(implantslot);
  } else {
    CHECK(!implantslot);
  }
  
  // implantitem must be buyable and have a slot of 1, but the rendering varies
  if(type == HNT_IMPLANTITEM || type == HNT_IMPLANTITEM_UPG) {  
    CHECK(!gottype);
    gottype = true;
    if(type == HNT_IMPLANTITEM) {
      CHECK(displaymode == HNDM_IMPLANT_EQUIP);
    } else if(type == HNT_IMPLANTITEM_UPG) {
      CHECK(displaymode == HNDM_IMPLANT_UPGRADE);
    } else {
      CHECK(0);
    }
    CHECK(buyable);
    CHECK(pack == 1);
    CHECK(implantitem);
  } else {
    CHECK(!implantitem);
  }
  
  // the "done" token has no cost or other display but is "buyable"
  if(type == HNT_DONE) {
    CHECK(!gottype);
    gottype = true;
    CHECK(displaymode == HNDM_BLANK);
    CHECK(buyable);
    CHECK(pack == 1);
    CHECK(name == "Done");
  }
  
  CHECK(gottype);
  
  // all things that are buyable have quantities
  if(buyable)
    CHECK(pack > 0);
  
  // if it's marked as UNIQUE, its quantity is 1
  if(displaymode == HNDM_COSTUNIQUE)
    CHECK(pack == 1);
  
  // it may have no restriction or a valid restriction
  CHECK(cat_restrictiontype == -1 || cat_restrictiontype >= 0 && cat_restrictiontype < HNT_LAST);
  
  // if it's not a category or an equip, it shouldn't have branches
  CHECK(type == HNT_CATEGORY || type == HNT_EQUIP || branches.size() == 0);
  
  // last, check the consistency of everything recursively
  for(int i = 0; i < branches.size(); i++) {
    if(cat_restrictiontype == HNT_IMPLANT_CAT) {
      CHECK(branches[i].type == HNT_IMPLANTSLOT || branches[i].type == HNT_IMPLANTITEM || branches[i].type == HNT_IMPLANTITEM_UPG);
      CHECK(branches[i].cat_restrictiontype == cat_restrictiontype);
    } else if(cat_restrictiontype != -1) {
      CHECK(branches[i].type == cat_restrictiontype || branches[i].type == HNT_CATEGORY);
      CHECK(branches[i].cat_restrictiontype == cat_restrictiontype);
    }
    branches[i].checkConsistency(errors);
  }
  //dprintf("Consistency scan leaving %s\n", name.c_str());
}

// Kin, ene, exp, trap, exot
// white blue orange cyan green

const Color nhcolor[] = { Color(0.7, 0.7, 0.8), Color(0.5, 0.5, 1.0), Color(0.9, 0.5, 0.2), Color(0.2, 0.7, 0.7), Color(0.5, 0.9, 0.2) };

Color HierarchyNode::getColor() const {
  if(type == HNT_WEAPON) {
    int dmg = -1;
    for(int i = 0; i < IDBAdjustment::DAMAGE_LAST; i++) {
      if(IDBWeaponAdjust(weapon, IDBAdjustment()).stats_damagePerSecondType(i) != 0) {
        if(dmg == -1) {
          dmg = i;
        } else {
          dmg = -2;
        }
      }
    }
    if(dmg < 0)
      return C::inactive_text;
    return nhcolor[dmg];
  } else {
    return C::inactive_text;
  }
  CHECK(0);
}
Color HierarchyNode::getHighlightColor() const {
  return C::active_text;
}

HierarchyNode::HierarchyNode() {
  type = HNT_LAST;
  displaymode = HNDM_LAST;
  buyable = false;
  pack = -1;
  cat_restrictiontype = -1;
  weapon = NULL;
  upgrade = NULL;
  glory = NULL;
  bombardment = NULL;
  tank = NULL;
  implantslot = NULL;
  implantitem = NULL;
  equipweapon = NULL;
  spawncash = Money(0);
}

bool isMountedNode(const string &in) {
  vector<string> toks = tokenize(in, ".");
  CHECK(toks.size());
  if(toks[0] != "ROOT")
    CHECK(toks.size() == 1);
  return toks[0] == "ROOT";
}

HierarchyNode *findNamedNode(const string &in, int postcut) {
  vector<string> toks = tokenize(in, ".");
  CHECK(toks.size());
  CHECK(toks.size() > postcut);
  toks.erase(toks.end() - postcut, toks.end());
  CHECK(toks[0] == "ROOT");
  HierarchyNode *current = &root;
  for(int i = 1; i < toks.size(); i++) {
    int fc = 0;
    int fi = -1;
    for(int k = 0; k < current->branches.size(); k++) {
      if(toks[i] == current->branches[k].name) {
        fc++;
        fi = k;
      }
    }
    if(fc == 0) {
      dprintf("Parent node not found for item hierarchy!\n");
    }
    CHECK(fc == 1);
    CHECK(fi != -1);
    current = &current->branches[fi];
  }
  return current;
}

bool prefixed(const string &label, const string &prefix) {
  if(label.size() < prefix.size() + 1)
    return false;
  if(label[prefix.size()] != '.')
    return false;
  if(strncmp(label.c_str(), prefix.c_str(), prefix.size()))
    return false;
  return true;
}

template<typename T> T *prepareName(kvData *chunk, map<string, T> *classes, bool reload, const string &prefix = "", string *namestorage = NULL) {
  string name = chunk->consume("name");
  if(namestorage)
    *namestorage = name;
  if(prefix.size())
    CHECK(prefixed(name, prefix));
  if(classes->count(name)) {
    if(!reload) {
      dprintf("Multiple definition of %s\n", name.c_str());
      CHECK(0);
    } else {
      (*classes)[name] = T();
    }
  }
  return &(*classes)[name];
}

template<typename T> T *prepareName(kvData *chunk, map<string, T> *classes, bool reload, string *namestorage) {
  return prepareName(chunk, classes, reload, "", namestorage);
}

template<typename T> const T *parseSubclass(const string &name, const map<string, T> &classes) {
  if(!classes.count(name)) {
    dprintf("Can't find token %s\n", name.c_str());
    CHECK(0);
  }
  return &classes.find(name)->second;
}

template<typename T> vector<const T *> parseSubclassSet(kvData *chunk, const string &name, const map<string, T> &classes) {
  if(!chunk->kv.count(name))
    return vector<const T *>();
  vector<const T *> types;
  vector<string> items = tokenize(chunk->consume(name), "\n");
  CHECK(items.size());
  for(int i = 0; i < items.size(); i++) {
    if(!classes.count(items[i])) {
      dprintf("Can't find object %s\n", items[i].c_str());
      CHECK(0);
    }
    types.push_back(&classes.find(items[i])->second);
  }
  return types;
}

template<typename T> const T *parseOptionalSubclass(kvData *chunk, const string &label, const map<string, T> &classes) {
  if(!chunk->kv.count(label))
    return NULL;
  return parseSubclass(chunk->consume(label), classes);
}

template<typename T> T parseSingleItem(const string &val);

template<> bool parseSingleItem<bool>(const string &val) {
  string lowered;
  for(int i = 0; i < val.size(); i++)
    lowered += val[i];
  if(lowered == "t" || lowered == "true")
    return true;
  if(lowered == "f" || lowered == "false")
    return false;
  CHECK(0);
}

template<> int parseSingleItem<int>(const string &val) {
  for(int i = 0; i < val.size(); i++)
    CHECK(isdigit(val[i]));
  return atoi(val.c_str());
}

template<> float parseSingleItem<float>(const string &val) {
  bool foundperiod = false;
  for(int i = 0; i < val.size(); i++) {
    if(val[i] == '-' && i == 0) {
    } else if(val[i] == '.') {
      CHECK(!foundperiod);
      foundperiod = true;
    } else {
      CHECK(isdigit(val[i]));
    }
  }
  return atof(val.c_str());
}

template<typename T> T parseWithDefault_processing(const string &val, T def) {
  CHECK(val.size());
  if(tolower(val[val.size() - 1]) == 'x') {
    CHECK(val.size() >= 2);
    CHECK(tolower(val[val.size() - 2]) != 'x');
    T mult = parseWithDefault_processing(string(val.begin(), val.end() - 1), T(1));
    return def * mult;
  }
  
  return parseSingleItem<T>(val);
}

template<> string parseWithDefault_processing<string>(const string &val, string def) {
  return val;
}
template<> Color parseWithDefault_processing<Color>(const string &val, Color def) {
  return colorFromString(val);
}
template<> Money parseWithDefault_processing<Money>(const string &val, Money def) {
  return moneyFromString(val);
}
  
template<typename T> T parseWithDefault(kvData *chunk, const string &label, T def) {
  if(!chunk->kv.count(label))
    return def;
  return parseWithDefault_processing(chunk->consume(label), def);
}

string parseWithDefault(kvData *chunk, const string &label, const char *def) {
  return parseWithDefault(chunk, label, string(def));
}
float parseWithDefault(kvData *chunk, const string &label, double def) {
  return parseWithDefault(chunk, label, float(def));
}

void parseDamagecode(const string &str, float *arr) {
  vector<string> toki = tokenize(str, "\n");
  CHECK(toki.size() >= 1);
  for(int i = 0; i < toki.size(); i++) {
    vector<string> qoki = tokenize(toki[i], " ");
    CHECK(qoki.size() == 2);
    int bucket = -1;
    if(qoki[1] == "kinetic")
      bucket = IDBAdjustment::DAMAGE_KINETIC;
    else if(qoki[1] == "energy")
      bucket = IDBAdjustment::DAMAGE_ENERGY;
    else if(qoki[1] == "explosive")
      bucket = IDBAdjustment::DAMAGE_EXPLOSIVE;
    else if(qoki[1] == "trap")
      bucket = IDBAdjustment::DAMAGE_TRAP;
    else if(qoki[1] == "exotic")
      bucket = IDBAdjustment::DAMAGE_EXOTIC;
    else
      CHECK(0);
    CHECK(arr[bucket] == 0);
    arr[bucket] = parseSingleItem<float>(qoki[0]);
  }
}

template<typename T> void doStandardPrereq(T *titem, const string &name, map<string, T> *classes) {
  
  titem->has_postreq = false;
  
  {
    string lastname = tokenize(name, " ").back();
    CHECK(lastname.size()); // if this is wrong something horrible has occured
    
    bool roman = true;
    for(int i = 0; i < lastname.size(); i++)
      if(lastname[i] != 'I' && lastname[i] != 'V' && lastname[i] != 'X')  // I figure nobody will get past X without tripping the checks earlier
        roman = false;
    
    if(lastname.size() == 0)
      roman = false;
    
    if(!roman) {
      titem->prereq = NULL;
    } else {
      
      int rv;
      for(rv = 0; rv < roman_max(); rv++)
        if(lastname == roman_number(rv))
          break;
      
      CHECK(rv < roman_max());
      
      if(rv == 0) {
        titem->prereq = NULL;
        return;
      }
      
      string locnam = string(name.c_str(), (const char*)strrchr(name.c_str(), ' ')) + " " + roman_number(rv - 1);
      
      CHECK(classes->count(locnam));
      CHECK(!(*classes)[locnam].has_postreq);
      titem->prereq = &(*classes)[locnam];
      (*classes)[locnam].has_postreq = true;
    }
  }
}

void parseHierarchy(kvData *chunk, bool reload, ErrorAccumulator &accum) {
  HierarchyNode *mountpoint = findNamedNode(chunk->kv["name"], 1);
  HierarchyNode tnode;
  tnode.name = tokenize(chunk->consume("name"), ".").back();
  dprintf("name: %s\n", tnode.name.c_str());
  tnode.type = HierarchyNode::HNT_CATEGORY;
  if(chunk->kv.count("pack")) {
    tnode.displaymode = HierarchyNode::HNDM_PACK;
    tnode.pack = atoi(chunk->consume("pack").c_str());
    CHECK(mountpoint->pack == -1);
  } else {
    tnode.displaymode = HierarchyNode::HNDM_BLANK;
    tnode.pack = mountpoint->pack;
  }
  if(chunk->kv.count("type")) {
    if(chunk->kv["type"] == "weapon") {
      tnode.cat_restrictiontype = HierarchyNode::HNT_WEAPON;
    } else if(chunk->kv["type"] == "upgrade") {
      tnode.cat_restrictiontype = HierarchyNode::HNT_UPGRADE;
    } else if(chunk->kv["type"] == "glory") {
      tnode.cat_restrictiontype = HierarchyNode::HNT_GLORY;
    } else if(chunk->kv["type"] == "bombardment") {
      tnode.cat_restrictiontype = HierarchyNode::HNT_BOMBARDMENT;
    } else if(chunk->kv["type"] == "tank") {
      tnode.cat_restrictiontype = HierarchyNode::HNT_TANK;
    } else if(chunk->kv["type"] == "implant") {
      tnode.cat_restrictiontype = HierarchyNode::HNT_IMPLANT_CAT;
    } else {
      dprintf("Unknown restriction type in hierarchy node: %s\n", chunk->kv["type"].c_str());
      CHECK(0);
    }
    chunk->consume("type");
  }
  if(tnode.cat_restrictiontype == -1) {
    tnode.cat_restrictiontype = mountpoint->cat_restrictiontype;
  }
  
  tnode.spawncash = parseWithDefault(chunk, "spawncash", Money(0));
  
  CHECK(mountpoint->cat_restrictiontype == -1 || tnode.cat_restrictiontype == mountpoint->cat_restrictiontype);
  mountpoint->branches.push_back(tnode);
}

void parseLauncher(kvData *chunk, bool reload, ErrorAccumulator &accum) {
  IDBLauncher *titem = prepareName(chunk, &launcherclasses, reload, "launcher");
  
  titem->deploy = parseSubclass(chunk->consume("deploy"), deployclasses);
  titem->stats = parseSubclass(chunk->consume("stats"), statsclasses);

  titem->text = parseOptionalSubclass(chunk, "text", text);
  
  string demotype = parseWithDefault(chunk, "demo", "firingrange");
  if(demotype == "firingrange") {
    titem->demomode = WDM_FIRINGRANGE;
    
    string distance = parseWithDefault(chunk, "firingrange_distance", "normal");
    
    if(distance == "normal") {
      titem->firingrange_distance = WFRD_NORMAL;
    } else if(distance == "melee") {
      titem->firingrange_distance = WFRD_MELEE;
    } else {
      CHECK(0);
    }
  } else if(demotype == "mines") {
    titem->demomode = WDM_MINES;
  } else {
    CHECK(0);
  }
}

void parseEffects(kvData *chunk, bool reload, ErrorAccumulator &accum) {
  IDBEffects *titem = prepareName(chunk, &effectsclasses, reload, "effects");
  
  titem->quantity = atoi(chunk->consume("quantity").c_str());
  
  titem->inertia = parseWithDefault(chunk, "inertia", 0.f);
  titem->reflect = parseWithDefault(chunk, "reflect", 0.f);
  
  titem->spread = atof(chunk->consume("spread").c_str());
  
  titem->slowdown = atof(chunk->consume("slowdown").c_str());
  titem->lifetime = atof(chunk->consume("lifetime").c_str());
  
  titem->radius = atof(chunk->consume("radius").c_str());
  titem->color = colorFromString(chunk->consume("color"));
}

void parseStats(kvData *chunk, bool reload, ErrorAccumulator &accum) {
  IDBStats *titem = prepareName(chunk, &statsclasses, reload, "stats");
    
  titem->dps_efficiency = parseWithDefault(chunk, "dps_efficiency", 1.0);
  titem->cps_efficiency = parseWithDefault(chunk, "cps_efficiency", 1.0);
}

void parseWeapon(kvData *chunk, bool reload, ErrorAccumulator &accum) {
  string name;
  IDBWeapon *titem = prepareName(chunk, &weaponclasses, reload, &name);
  
  const string informal_name = tokenize(name, ".").back();
  
  if(isMountedNode(name)) {
    HierarchyNode *mountpoint = findNamedNode(name, 1);
    HierarchyNode tnode;
    tnode.name = informal_name;
    tnode.type = HierarchyNode::HNT_WEAPON;
    tnode.displaymode = HierarchyNode::HNDM_COST;
    tnode.buyable = true;
    tnode.pack = mountpoint->pack;
    tnode.cat_restrictiontype = HierarchyNode::HNT_WEAPON;
    CHECK(mountpoint->cat_restrictiontype == -1 || tnode.cat_restrictiontype == mountpoint->cat_restrictiontype);
    tnode.weapon = titem;
    tnode.spawncash = parseWithDefault(chunk, "spawncash", Money(0));
    mountpoint->branches.push_back(tnode);
    
    CHECK(mountpoint->pack >= 1);
    titem->quantity = mountpoint->pack;
    titem->base_cost = moneyFromString(chunk->consume("cost"));
    CHECK(titem->base_cost > Money(0));
  } else {
    CHECK(!chunk->kv.count("cost"));
    titem->quantity = 100; // why not?
    titem->base_cost = Money(0);
  }
  
  titem->name = informal_name;
  titem->firerate = atof(chunk->consume("firerate").c_str());
  
  titem->launcher = parseSubclass(chunk->consume("launcher"), launcherclasses);
  CHECK(titem->launcher->stats);
  
  titem->nocache = parseWithDefault(chunk, "nocache", false);
}

void parseUpgrade(kvData *chunk, bool reload, ErrorAccumulator &accum) {
  // This one turns out to be rather complicated.
  string name;
  string category;
  IDBUpgrade *titem;
  {
    name = chunk->consume("name");
    category = chunk->consume("category");
    string locnam = category + "+" + name;
    if(upgradeclasses.count(locnam)) {
      if(!reload) {
        dprintf("Multiple definition of %s\n", locnam.c_str());
        CHECK(0);
      } else {
        upgradeclasses[locnam] = IDBUpgrade();
      }
    }
    titem = &upgradeclasses[locnam];
  }
  
  titem->costmult = atof(chunk->consume("costmult").c_str());
  
  titem->adjustment = parseSubclass(chunk->consume("adjustment"), adjustmentclasses);
  titem->category = category;
  
  titem->text = parseOptionalSubclass(chunk, "text", text);
  
  doStandardPrereq(titem, category + "+" + name, &upgradeclasses);
  
  {
    HierarchyNode *mountpoint = findNamedNode(name, 1);
    HierarchyNode tnode;
    tnode.name = tokenize(name, ".").back();
    tnode.type = HierarchyNode::HNT_UPGRADE;
    tnode.displaymode = HierarchyNode::HNDM_COSTUNIQUE;
    tnode.buyable = true;
    tnode.pack = 1;
    CHECK(mountpoint->pack == 1 || mountpoint->pack == -1);
    tnode.cat_restrictiontype = HierarchyNode::HNT_UPGRADE;
    CHECK(mountpoint->cat_restrictiontype == -1 || tnode.cat_restrictiontype == mountpoint->cat_restrictiontype);
    tnode.upgrade = titem;
    mountpoint->branches.push_back(tnode);
  }
}

void parseProjectile(kvData *chunk, bool reload, ErrorAccumulator &accum) {
  IDBProjectile *titem = prepareName(chunk, &projectileclasses, reload, "projectile");
  
  titem->thickness_visual = parseWithDefault(chunk, "thickness_visual", 0.3);
  
  string motion = parseWithDefault(chunk, "motion", "normal");
  if(motion == "normal") {
    titem->motion = PM_NORMAL;
  } else if(motion == "missile") {
    titem->motion = PM_MISSILE;
    titem->missile_stabstart = parseSingleItem<float>(chunk->consume("missile_stabstart"));
    titem->missile_stabilization = parseSingleItem<float>(chunk->consume("missile_stabilization"));
    titem->missile_sidelaunch = parseSingleItem<float>(chunk->consume("missile_sidelaunch"));
    titem->missile_backlaunch = parseSingleItem<float>(chunk->consume("missile_backlaunch"));
  } else if(motion == "airbrake") {
    titem->motion = PM_AIRBRAKE;
    titem->airbrake_life = parseWithDefault(chunk, "airbrake_life", 1.0);
  } else if(motion == "mine") {
    titem->motion = PM_MINE;
    titem->radius_physical = atof(chunk->consume("radius_physical").c_str());
    titem->halflife = atof(chunk->consume("halflife").c_str());
  } else {
    dprintf("Unknown projectile motion: %s\n", motion.c_str());
    CHECK(0);
  }
  
  titem->color = parseWithDefault(chunk, "color", C::gray(1.0));
  
  titem->velocity = 0;
  if(titem->motion != PM_MINE)
    titem->velocity = atof(chunk->consume("velocity").c_str());
  
  if(titem->motion == PM_NORMAL) {
    titem->length = parseWithDefault(chunk, "length", titem->velocity / 60);
  } else if(titem->motion == PM_MISSILE) {
    titem->length = parseWithDefault(chunk, "length", 2.0);
  } else if(titem->motion == PM_AIRBRAKE || titem->motion == PM_MINE) {
    titem->length = 0;
  } else {
    CHECK(0);
  }
  
  titem->chain_warhead = parseSubclassSet(chunk, "warhead", warheadclasses);
  
  titem->toughness = parseWithDefault(chunk, "toughness", 1.0);
  
  CHECK(titem->chain_warhead.size());
}

void parseDeploy(kvData *chunk, bool reload, ErrorAccumulator &accum) {
  IDBDeploy *titem = prepareName(chunk, &deployclasses, reload, "deploy");
  
  string type = parseWithDefault(chunk, "type", "normal");
  
  if(type == "normal") {
    titem->type = DT_NORMAL;
  } else if(type == "forward") {
    titem->type = DT_FORWARD;
  } else if(type == "centroid") {
    titem->type = DT_CENTROID;
  } else if(type == "minepath") {
    titem->type = DT_MINEPATH;
  } else if(type == "explode") {
    titem->type = DT_EXPLODE;
    titem->exp_minsplits = atoi(chunk->consume("exp_minsplits").c_str());
    titem->exp_maxsplits = atoi(chunk->consume("exp_maxsplits").c_str());
    titem->exp_minsplitsize = atoi(chunk->consume("exp_minsplitsize").c_str());
    titem->exp_maxsplitsize = atoi(chunk->consume("exp_maxsplitsize").c_str());
    titem->exp_shotspersplit = atoi(chunk->consume("exp_shotspersplit").c_str());
  } else {
    CHECK(0);
  }
  
  titem->anglestddev = parseWithDefault(chunk, "anglestddev", 0.0);
  
  titem->chain_deploy = parseSubclassSet(chunk, "deploy", deployclasses);
  titem->chain_projectile = parseSubclassSet(chunk, "projectile", projectileclasses);
  titem->chain_warhead = parseSubclassSet(chunk, "warhead", warheadclasses);
  for(int i = 0; i < titem->chain_deploy.size(); i++)
    CHECK(titem->chain_deploy[i] != titem);
  
  CHECK(titem->chain_deploy.size() || titem->chain_projectile.size() || titem->chain_warhead.size());
}

void parseWarhead(kvData *chunk, bool reload, ErrorAccumulator &accum) {
  IDBWarhead *titem = prepareName(chunk, &warheadclasses, reload, "warhead");
  
  memset(titem->impactdamage, 0, sizeof(titem->impactdamage));
  memset(titem->radiusdamage, 0, sizeof(titem->radiusdamage));
  
  // these must either neither exist, or both exist
  CHECK(chunk->kv.count("radiusfalloff") == chunk->kv.count("radiusdamage"));
  CHECK(chunk->kv.count("radiuscolor_bright") == chunk->kv.count("radiuscolor_dim"));
  
  // if wallremovalchance exists, wallremovalradius must too
  CHECK(chunk->kv.count("wallremovalchance") <= chunk->kv.count("wallremovalradius"));
  
  // if radiuscolor_bright exists, radiusfalloff must too
  CHECK(chunk->kv.count("radiuscolor_bright") <= chunk->kv.count("radiusfalloff"));
  
  titem->radiuscolor_bright = parseWithDefault(chunk, "radiuscolor_bright", Color(1.0, 0.8, 0.2));
  titem->radiuscolor_dim = parseWithDefault(chunk, "radiuscolor_dim", Color(1.0, 0.2, 0.0));
  
  if(chunk->kv.count("impactdamage"))
    parseDamagecode(chunk->consume("impactdamage"), titem->impactdamage);
  if(chunk->kv.count("radiusdamage"))
    parseDamagecode(chunk->consume("radiusdamage"), titem->radiusdamage);
  
  titem->radiusfalloff = parseWithDefault(chunk, "radiusfalloff", -1.0);
  
  titem->wallremovalradius = parseWithDefault(chunk, "wallremovalradius", 0.0);
  titem->wallremovalchance = parseWithDefault(chunk, "wallremovalchance", 1.0);
  
  titem->deploy = parseSubclassSet(chunk, "deploy", deployclasses);
  
  titem->effects_impact = parseSubclassSet(chunk, "effects_impact", effectsclasses);
}

void parseGlory(kvData *chunk, bool reload, ErrorAccumulator &accum) {
  string name;
  IDBGlory *titem = prepareName(chunk, &gloryclasses, reload, &name);
  
  titem->base_cost = moneyFromString(chunk->consume("cost"));
  
  titem->blast = parseSubclassSet(chunk, "blast", deployclasses);
  titem->core = parseSubclass(chunk->consume("core"), deployclasses);
  
  if(chunk->kv.count("default") && atoi(chunk->consume("default").c_str())) {
    CHECK(!defglory);
    defglory = titem;
  }
  
  titem->demo_range = parseWithDefault(chunk, "demo_range", 100);
  
  titem->text = parseOptionalSubclass(chunk, "text", text);
  
  {
    HierarchyNode *mountpoint = findNamedNode(name, 1);
    HierarchyNode tnode;
    tnode.name = tokenize(name, ".").back();
    tnode.type = HierarchyNode::HNT_GLORY;
    tnode.displaymode = HierarchyNode::HNDM_COSTUNIQUE;
    tnode.buyable = true;
    tnode.pack = 1;
    tnode.cat_restrictiontype = HierarchyNode::HNT_GLORY;
    CHECK(mountpoint->cat_restrictiontype == -1 || tnode.cat_restrictiontype == mountpoint->cat_restrictiontype);

    tnode.glory = titem;
    tnode.spawncash = titem->base_cost / 2;
    mountpoint->branches.push_back(tnode);
  }
}

void parseBombardment(kvData *chunk, bool reload, ErrorAccumulator &accum) {
  string name;
  IDBBombardment *titem = prepareName(chunk, &bombardmentclasses, reload, &name);
  
  titem->warhead = parseSubclass(chunk->consume("warhead"), warheadclasses);
  
  titem->base_cost = moneyFromString(chunk->consume("cost"));

  titem->lockdelay = atof(chunk->consume("lockdelay").c_str());
  titem->unlockdelay = atof(chunk->consume("unlockdelay").c_str());

  if(chunk->kv.count("default") && atoi(chunk->consume("default").c_str())) {
    CHECK(!defbombardment);
    defbombardment = titem;
  }
  
  titem->text = parseOptionalSubclass(chunk, "text", text);
  
  {
    HierarchyNode *mountpoint = findNamedNode(name, 1);
    HierarchyNode tnode;
    tnode.name = tokenize(name, ".").back();
    tnode.type = HierarchyNode::HNT_BOMBARDMENT;
    tnode.displaymode = HierarchyNode::HNDM_COSTUNIQUE;
    tnode.buyable = true;
    tnode.pack = 1;
    tnode.cat_restrictiontype = HierarchyNode::HNT_BOMBARDMENT;
    CHECK(mountpoint->cat_restrictiontype == -1 || tnode.cat_restrictiontype == mountpoint->cat_restrictiontype);
    
    tnode.spawncash = titem->base_cost / 2;
    tnode.bombardment = titem;
    mountpoint->branches.push_back(tnode);
  }
}

void parseTank(kvData *chunk, bool reload, ErrorAccumulator &accum) {
  string name;
  IDBTank *titem = prepareName(chunk, &tankclasses, reload, &name);
  
  string weapon = chunk->consume("weapon");
  if(!weaponclasses.count(weapon))
    dprintf("Can't find weapon %s", weapon.c_str());
  CHECK(weaponclasses.count(weapon));
  
  titem->weapon = &weaponclasses[weapon];
  
  titem->health = atof(chunk->consume("health").c_str());
  titem->handling = atof(chunk->consume("handling").c_str());
  titem->engine = atof(chunk->consume("engine").c_str());
  titem->mass = atof(chunk->consume("mass").c_str());
  
  titem->adjustment = parseOptionalSubclass(chunk, "adjustment", adjustmentclasses);
  titem->upgrades = tokenize(chunk->consume("upgrades"), "\n");
  
  {
    vector<string> vtx = tokenize(chunk->consume("vertices"), "\n");
    CHECK(vtx.size() >= 3); // triangle is the minimum, no linetanks please
    bool got_firepoint = false;
    bool in_mine_path = false;
    for(int i = 0; i < vtx.size(); i++) {
      vector<string> vti = tokenize(vtx[i], " ");
      CHECK(vti.size() == 2 || vti.size() == 3);
      Coord2 this_vertex = Coord2(atof(vti[0].c_str()), atof(vti[1].c_str()));
      titem->vertices.push_back(Coord2(this_vertex));
      if(in_mine_path)
        titem->minepath.push_back(this_vertex);
      if(vti.size() == 3) {
        if(vti[2] == "firepoint") {
          CHECK(!got_firepoint);
          titem->firepoint = this_vertex;
          got_firepoint = true;
        } else if(vti[2] == "rear_begin") {
          CHECK(!in_mine_path);
          CHECK(!titem->minepath.size());
          in_mine_path = true;
          titem->minepath.push_back(this_vertex);
        } else if(vti[2] == "rear_end") {
          CHECK(in_mine_path);
          CHECK(titem->minepath.size());
          in_mine_path = false;
        } else {
          CHECK(0);
        }
      }
    }
    CHECK(titem->minepath.size() >= 2);
    
    titem->centering_adjustment = getCentroid(titem->vertices);
    for(int i = 0; i < titem->vertices.size(); i++)
      titem->vertices[i] -= titem->centering_adjustment;
    titem->firepoint -= titem->centering_adjustment;
    for(int i = 0; i < titem->minepath.size(); i++)
      titem->minepath[i] -= titem->centering_adjustment;
  }
  
  titem->base_cost = moneyFromString(chunk->consume("cost"));
  
  titem->upgrade_base = parseWithDefault(chunk, "upgrade_base", titem->base_cost);
  
  if(chunk->kv.count("default") && atoi(chunk->consume("default").c_str())) {
    CHECK(!deftank);
    deftank = titem;
  }
  
  titem->text = parseOptionalSubclass(chunk, "text", text);
  
  {
    HierarchyNode *mountpoint = findNamedNode(name, 1);
    HierarchyNode tnode;
    tnode.name = tokenize(name, ".").back();
    tnode.type = HierarchyNode::HNT_TANK;
    tnode.displaymode = HierarchyNode::HNDM_COSTUNIQUE;
    tnode.buyable = true;
    tnode.pack = 1;
    tnode.cat_restrictiontype = HierarchyNode::HNT_TANK;
    CHECK(mountpoint->cat_restrictiontype == -1 || tnode.cat_restrictiontype == mountpoint->cat_restrictiontype);
    
    tnode.spawncash = titem->base_cost / 2;
    tnode.tank = titem;
    mountpoint->branches.push_back(tnode);
  }
}

void parseAdjustment(kvData *chunk, bool reload, ErrorAccumulator &accum) {
  IDBAdjustment *titem = prepareName(chunk, &adjustmentclasses, reload, "adjustment");
  
  CHECK(ARRAY_SIZE(adjust_text) == IDBAdjustment::COMBO_LAST);
  CHECK(ARRAY_SIZE(adjust_human) == IDBAdjustment::COMBO_LAST);
  CHECK(ARRAY_SIZE(adjust_unit) == IDBAdjustment::LAST);
  
  int cps = 0;
  
  for(int i = 0; i < IDBAdjustment::LAST; i++) {
    if(chunk->kv.count(adjust_text[i])) {
      CHECK(cps < ARRAY_SIZE(titem->adjustlist));
      int value = atoi(chunk->consume(adjust_text[i]).c_str());
      titem->adjusts[i] = value;
      titem->adjustlist[cps++] = make_pair(i, value);
    }
  }
  
  for(int i = IDBAdjustment::LAST; i < IDBAdjustment::COMBO_LAST; i++) {
    if(chunk->kv.count(adjust_text[i])) {
      CHECK(cps < ARRAY_SIZE(titem->adjustlist));
      int value = atoi(chunk->consume(adjust_text[i]).c_str());
      titem->adjustlist[cps++] = make_pair(i, value);
      if(i == IDBAdjustment::DAMAGE_ALL) {
        for(int j = 0; j < IDBAdjustment::DAMAGE_LAST; j++) {
          CHECK(titem->adjusts[j] == 0);
          titem->adjusts[j] = value;
        }
      } else if(i == IDBAdjustment::ALL) {
        for(int j = 0; j < IDBAdjustment::LAST; j++) {
          CHECK(titem->adjusts[j] == 0);
          titem->adjusts[j] = value;
        }
      } else {
        CHECK(0);
      }
    }
  }
}

void parseFaction(kvData *chunk, bool reload, ErrorAccumulator &accum) {
  IDBFaction fact;
  
  fact.icon = loadDvec2("data/base/faction_icons/" + chunk->consume("file"));
  fact.color = colorFromString(chunk->consume("color"));
  fact.name = chunk->consume("name");
  
  {
    vector<int> lines = sti(tokenize(chunk->consume("lines"), " "));
    vector<string> words = tokenize(fact.name, " ");
    CHECK(words.size() == accumulate(lines.begin(), lines.end(), 0));
    int cword = 0;
    for(int i = 0; i < lines.size(); i++) {
      string acu;
      for(int j = 0; j < lines[i]; j++) {
        if(j)
          acu += " ";
        acu += words[cword++];
      }
      fact.name_lines.push_back(acu);
    }
  }
  
  adjustmentclasses["null"]; // this is a hideous hack just FYI
  for(int i = 0; i < 3; i++)
    fact.adjustment[i] = parseSubclass("null", adjustmentclasses); // wheeeeeeeee
  fact.adjustment[3] = parseSubclass(chunk->consume("adjustment") +  ".high", adjustmentclasses);
  
  fact.text = parseOptionalSubclass(chunk, "text", text);
  
  factions.push_back(fact);
}

void parseText(kvData *chunk, bool reload, ErrorAccumulator &accum) {
  string *titem = prepareName(chunk, &text, reload, "text");
  *titem = chunk->consume("data");
  // yay
}

void parseShopcache(kvData *chunk, vector<string> *errors) {
  IDBShopcache *titem = prepareName(chunk, &shopcaches, false);
  
  vector<string> tse = tokenize(chunk->consume("x"), "\n");
  for(int i = 0; i < tse.size(); i++) {
    IDBShopcache::Entry entry;
    vector<string> tis = tokenize(tse[i], " ");
    CHECK(tis.size() >= 3);
    CHECK(tis.size() % 2 == 1);
    entry.count = atoi(tis[0].c_str());
    entry.warhead = &warheadclasses[tis[1]];
    if(tis[2] == "none") {
      entry.impact = -1;
    } else {
      entry.impact = atoi(tis[2].c_str());
    }
    for(int i = 3; i < tis.size(); i += 2) {
      entry.distances.push_back(make_pair(floatFromString(tis[i]), atoi(tis[i + 1].c_str())));
    }
    titem->entries.push_back(entry);
  }
  
  titem->cycles = atoi(chunk->consume("cycles").c_str());
  
  vector<string> tspec = tokenize(chunk->consume("tankstats"), "\n");
  for(int i = 0; i < tspec.size(); i++) {
    vector<int> ti = sti(tokenize(tspec[i], " "));
    CHECK(ti.size() == 2);
    CHECK(ti[0] == i);
    titem->tank_specific.push_back(ti[1]);
  }
}

void parseImplantSlot(kvData *chunk, bool reload, ErrorAccumulator &accum) {
  string name;
  IDBImplantSlot *titem = prepareName(chunk, &implantslotclasses, reload, &name);
  
  titem->cost = moneyFromString(chunk->consume("cost"));
  
  titem->text = parseOptionalSubclass(chunk, "text", text);
  
  doStandardPrereq(titem, name, &implantslotclasses);
  
  {
    HierarchyNode *mountpoint = findNamedNode(name, 1);
    HierarchyNode tnode;
    tnode.name = tokenize(name, ".").back();
    tnode.type = HierarchyNode::HNT_IMPLANTSLOT;
    tnode.displaymode = HierarchyNode::HNDM_COSTUNIQUE;
    tnode.buyable = true;
    tnode.pack = 1;
    tnode.cat_restrictiontype = HierarchyNode::HNT_IMPLANT_CAT;
    CHECK(mountpoint->cat_restrictiontype == -1 || tnode.cat_restrictiontype == mountpoint->cat_restrictiontype);
    
    tnode.implantslot = titem;
    mountpoint->branches.push_back(tnode);
  }
}

void parseImplant(kvData *chunk, bool reload, ErrorAccumulator &accum) {
  string name;
  IDBImplant *titem = prepareName(chunk, &implantclasses, reload, &name);
  
  titem->adjustment = parseSubclass(chunk->consume("adjustment"), adjustmentclasses);
  
  titem->text = parseOptionalSubclass(chunk, "text", text);
  
  // this is kind of grim - we push two nodes in. This could happen at shop manipulation time also, I suppose.
  {
    HierarchyNode *mountpoint = findNamedNode(name, 1);
    HierarchyNode tnode;
    tnode.name = tokenize(name, ".").back();
    tnode.type = HierarchyNode::HNT_IMPLANTITEM;
    tnode.displaymode = HierarchyNode::HNDM_IMPLANT_EQUIP;
    tnode.buyable = true;
    tnode.pack = 1;
    tnode.cat_restrictiontype = HierarchyNode::HNT_IMPLANT_CAT;
    CHECK(mountpoint->cat_restrictiontype == -1 || tnode.cat_restrictiontype == mountpoint->cat_restrictiontype);
    
    tnode.implantitem = titem;
    mountpoint->branches.push_back(tnode);
  }
  
  {
    HierarchyNode *mountpoint = findNamedNode(name, 1);
    HierarchyNode tnode;
    tnode.name = tokenize(name, ".").back();
    tnode.type = HierarchyNode::HNT_IMPLANTITEM_UPG;
    tnode.displaymode = HierarchyNode::HNDM_IMPLANT_UPGRADE;
    tnode.buyable = true;
    tnode.pack = 1;
    tnode.cat_restrictiontype = HierarchyNode::HNT_IMPLANT_CAT;
    CHECK(mountpoint->cat_restrictiontype == -1 || tnode.cat_restrictiontype == mountpoint->cat_restrictiontype);
    
    tnode.spawncash = Money(75000);
    tnode.implantitem = titem;
    mountpoint->branches.push_back(tnode);
  }
}

void parseItemFile(const string &fname, bool reload, vector<string> *errors) {
  ifstream tfil(fname.c_str());
  CHECK(tfil);
  
  int line = 0;
  int nextline = 0;
  
  kvData chunk;
  while(getkvData(tfil, &chunk, &line, &nextline)) {
    //dprintf("%s\n", chunk.debugOutput().c_str());
    if(parseWithDefault(&chunk, "debug", false) && !FLAGS_debugitems) {
      dprintf("Debug only, skipping\n");
      continue;
    }
    ErrorAccumulator erac(errors, fname, line);
    if(chunk.category == "hierarchy") {
      parseHierarchy(&chunk, reload, erac);
    } else if(chunk.category == "weapon") {
      parseWeapon(&chunk, reload, erac);
    } else if(chunk.category == "upgrade") {
      parseUpgrade(&chunk, reload, erac);
    } else if(chunk.category == "projectile") {
      parseProjectile(&chunk, reload, erac);
    } else if(chunk.category == "deploy") {
      parseDeploy(&chunk, reload, erac);
    } else if(chunk.category == "warhead") {
      parseWarhead(&chunk, reload, erac);
    } else if(chunk.category == "glory") {
      parseGlory(&chunk, reload, erac);
    } else if(chunk.category == "bombardment") {
      parseBombardment(&chunk, reload, erac);
    } else if(chunk.category == "tank") {
      parseTank(&chunk, reload, erac);
    } else if(chunk.category == "adjustment") {
      parseAdjustment(&chunk, reload, erac);
    } else if(chunk.category == "faction") {
      parseFaction(&chunk, reload, erac);
    } else if(chunk.category == "text") {
      parseText(&chunk, reload, erac);
    } else if(chunk.category == "launcher") {
      parseLauncher(&chunk, reload, erac);
    } else if(chunk.category == "stats") {
      parseStats(&chunk, reload, erac);
    } else if(chunk.category == "effects") {
      parseEffects(&chunk, reload, erac);
    } else if(chunk.category == "implantslot") {
      parseImplantSlot(&chunk, reload, erac);
    } else if(chunk.category == "implant") {
      parseImplant(&chunk, reload, erac);
    } else {
      CHECK(0);
    }
    if(!chunk.isDone())
      erac.addError(StringPrintf("Chunk still has unparsed data! %s", chunk.debugOutput().c_str()));
  }
}

void idb_coreinit() {
  CHECK(root.name == "");
  root.name = "ROOT";
  root.type = HierarchyNode::HNT_CATEGORY;
  root.displaymode = HierarchyNode::HNDM_BLANK;
}

void loadItemDb(bool reload) {
  
  idb_coreinit();
  
  vector<string> errors;
  
  string basepath = "data/base/";
  ifstream manifest((basepath + "manifest").c_str());
  string line;
  while(getLineStripped(manifest, &line)) {
    dprintf("%s\n", line.c_str());
    parseItemFile(basepath + line, reload, &errors);
  }
  
  // add our hardcoded "sell" token
  {
    HierarchyNode tnode;
    tnode.name = "Sell equipment";
    tnode.type = HierarchyNode::HNT_SELL;
    tnode.displaymode = HierarchyNode::HNDM_BLANK;
    tnode.cat_restrictiontype = HierarchyNode::HNT_NONE;
    root.branches.push_back(tnode);
  }
  
  // add our hardcoded "equip" token
  {
    HierarchyNode tnode;
    tnode.name = "Equip weapons";
    tnode.type = HierarchyNode::HNT_EQUIP;
    tnode.displaymode = HierarchyNode::HNDM_BLANK;
    tnode.cat_restrictiontype = HierarchyNode::HNT_EQUIPWEAPON;
    root.branches.push_back(tnode);
  }
  
  {
    // add our hardcoded "done" token
    HierarchyNode tnode;
    tnode.name = "Done";
    tnode.type = HierarchyNode::HNT_DONE;
    tnode.displaymode = HierarchyNode::HNDM_BLANK;
    tnode.buyable = true;
    tnode.pack = 1;
    root.branches.push_back(tnode);
  }
  
  if(FLAGS_shopcache) {
    ifstream shopcache("data/shopcache.dwh");
    if(shopcache) {
      dprintf("Loading shop cache");
      kvData chunk;
      while(getkvData(shopcache, &chunk)) {
        CHECK(chunk.category == "shopcache");
        parseShopcache(&chunk, &errors);
        if(!chunk.isDone()) {
          dprintf("Chunk still has unparsed data!\n");
          dprintf("%s\n", chunk.debugOutput().c_str());
          CHECK(0);
        }
      }
    } else {
      dprintf("No shop cache available, skipping");
    }
  }
  
  dprintf("done loading, consistency check\n");
  CHECK(deftank);
  CHECK(defglory);
  CHECK(defbombardment);
  root.checkConsistency(&errors);
  
  if(errors.size() == 0) {
    dprintf("EVERYTHING IS AWESOME! HELLS FUCKING YES\n");
  } else {
    dprintf("%d errors\n", errors.size());
    for(int i = 0; i < errors.size(); i++)
      dprintf("    %s\n", errors[i].c_str());
    CHECK(0);
  }
}

void addItemFile(const string &file) {
  if(root.name == "")
    idb_coreinit();
  
  vector<string> errors;
  
  parseItemFile(file, false, &errors);
  
  if(errors.size() != 0) {
    dprintf("%d errors\n", errors.size());
    for(int i = 0; i < errors.size(); i++)
      dprintf("    %s\n", errors[i].c_str());
    CHECK(0);
  }
}

void reloadItemdb() {
  root = HierarchyNode();
  
  vector<IDBFaction *> idfa;
  for(int i = 0; i < factions.size(); i++)
    idfa.push_back(&factions[i]);
  factions.clear();
  
  deftank = NULL;
  defglory = NULL;
  defbombardment = NULL;
  
  shopcaches.clear();
  
  loadItemDb(true);
  
  CHECK(factions.size() == idfa.size());
  for(int i = 0; i < factions.size(); i++)
    CHECK(idfa[i] == &factions[i]);
}

void initItemdb() {
  CHECK(root.name == "");
  
  loadItemDb(false);
}

IDBAdjustment IDBImplant::makeAdjustment(int level) const {
  CHECK(level > 0);
  return *adjustment * level;
}

vector<Coord2> IDBTank::getTankVertices(Coord2 pos, float td) const {
  Coord2 xt = makeAngle(Coord(td));
  Coord2 yt = makeAngle(Coord(td) - COORDPI / 2);
  vector<Coord2> rv;
  for(int i = 0; i < vertices.size(); i++)
    rv.push_back(Coord2(pos.x + vertices[i].x * xt.x + vertices[i].y * xt.y, pos.y + vertices[i].y * yt.y + vertices[i].x * yt.x));
  return rv;
}

const HierarchyNode &itemDbRoot() {
  return root;
}

const IDBTank *defaultTank() {
  CHECK(deftank);
  return deftank;
}
const IDBGlory *defaultGlory() {
  CHECK(defglory);
  return defglory;
}
const IDBBombardment *defaultBombardment() {
  CHECK(defbombardment);
  return defbombardment;
}

const map<string, IDBWeapon> &weaponList() {
  return weaponclasses;
}
const map<string, IDBBombardment> &bombardmentList() {
  return bombardmentclasses;
}
const map<string, IDBGlory> &gloryList() {
  return gloryclasses;
}
const map<string, IDBTank> &tankList() {
  return tankclasses;
}
const vector<IDBFaction> &factionList() {
  return factions;
}
const map<string, string> &textList() {
  return text;
}

bool hasShopcache(const IDBWeapon *weap) {
  return shopcaches.count(nameFromIDB(weap));
}
bool hasShopcache(const IDBBombardment *bombard) {
  return shopcaches.count(nameFromIDB(bombard));
}
bool hasShopcache(const IDBGlory *glory) {
  return shopcaches.count(nameFromIDB(glory));
}

const IDBShopcache &getShopcache(const IDBWeapon *weap) {
  CHECK(hasShopcache(weap));
  return shopcaches[nameFromIDB(weap)];
}
const IDBShopcache &getShopcache(const IDBBombardment *bombard) {
  CHECK(hasShopcache(bombard));
  return shopcaches[nameFromIDB(bombard)];
}
const IDBShopcache &getShopcache(const IDBGlory *glory) {
  CHECK(hasShopcache(glory));
  return shopcaches[nameFromIDB(glory)];
}

template<typename T> const string &nameFromIDBImp(const T *idbw, const map<string, T> &forward, map<const T*, string> *backward) {
  if(forward.size() != backward->size()) {
    backward->clear();
    for(typename map<string, T>::const_iterator itr = forward.begin(); itr != forward.end(); itr++) {
      CHECK(!backward->count(&itr->second));
      (*backward)[&itr->second] = itr->first;
    }
    CHECK(forward.size() == backward->size());
  }
  
  CHECK(backward->count(idbw));
  CHECK(forward.count((*backward)[idbw]));
  CHECK(&forward.find((*backward)[idbw])->second == idbw);
  return (*backward)[idbw];
}

static map<const IDBWeapon *, string> weaponclasses_reverse;
const string &nameFromIDB(const IDBWeapon *idbw) {
  return nameFromIDBImp(idbw, weaponclasses, &weaponclasses_reverse);
}

static map<const IDBWarhead *, string> warheadclasses_reverse;
const string &nameFromIDB(const IDBWarhead *idbw) {
  return nameFromIDBImp(idbw, warheadclasses, &warheadclasses_reverse);
}

static map<const IDBGlory *, string> gloryclasses_reverse;
const string &nameFromIDB(const IDBGlory *idbw) {
  return nameFromIDBImp(idbw, gloryclasses, &gloryclasses_reverse);
}

static map<const IDBBombardment *, string> bombardmentclasses_reverse;
const string &nameFromIDB(const IDBBombardment *idbw) {
  return nameFromIDBImp(idbw, bombardmentclasses, &bombardmentclasses_reverse);
}
