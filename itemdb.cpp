
#include "itemdb.h"

#include "args.h"
#include "parse.h"
#include "player.h"
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

static vector<IDBFaction> factions;
static map<string, string> text;

static const IDBTank *deftank = NULL;
static const IDBGlory *defglory = NULL;
static const IDBBombardment *defbombardment = NULL;

DEFINE_bool(debugitems, false, "Enable debug items");

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

void IDBAdjustment::debugDump() {
  dprintf("IDBAdjustment debug dump");
  for(int i = 0; i < LAST; i++)
    if(adjusts[i])
      dprintf("%12s: %d", adjust_text[i], adjusts[i]);
}

IDBAdjustment::IDBAdjustment() {
  memset(adjusts, 0, sizeof(adjusts));
  memset(adjustlist, -1, sizeof(adjustlist));
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
  return (float)(adjusts[type] + 100) / 100;
}

float IDBAdjustment::recyclevalue() const {
  float shares = 1 * adjustmentfactor(IDBAdjustment::RECYCLE_BONUS);
  float ratio = shares / (shares + 1.0);
  return ratio;
}

Money HierarchyNode::cost(const Player *player) const {
  if(type == HNT_WEAPON) {
    return player->adjustWeapon(weapon).cost();
  } else if(type == HNT_UPGRADE) {
    return player->adjustUpgradeForCurrentTank(upgrade).cost();
  } else if(type == HNT_GLORY) {
    return player->adjustGlory(glory).cost();
  } else if(type == HNT_BOMBARDMENT) {
    return player->adjustBombardment(bombardment).cost();
  } else if(type == HNT_TANK) {
    return player->adjustTankWithInstanceUpgrades(tank).cost();
  } else {
    CHECK(0);
  }
}

Money HierarchyNode::sellvalue(const Player *player) const {
  if(type == HNT_WEAPON) {
    return player->adjustWeapon(weapon).sellcost(min(pack, player->ammoCount(weapon)));
  } else if(type == HNT_GLORY) {
    return player->adjustGlory(glory).sellcost();
  } else if(type == HNT_BOMBARDMENT) {
    return player->adjustBombardment(bombardment).sellcost();
  } else if(type == HNT_TANK) {
    return player->sellTankValue(tank);
  } else {
    CHECK(0);
  }
}

void HierarchyNode::checkConsistency() const {
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
    if(cat_restrictiontype != -1) {
      CHECK(branches[i].type == cat_restrictiontype || branches[i].type == HNT_CATEGORY);
      CHECK(branches[i].cat_restrictiontype == cat_restrictiontype);
    }
    branches[i].checkConsistency();
  }
  //dprintf("Consistency scan leaving %s\n", name.c_str());
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
  CHECK(classes.count(name));
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

template<> int parseSingleItem<int>(const string &val) {
  for(int i = 0; i < val.size(); i++)
    CHECK(isdigit(val[i]));
  return atoi(val.c_str());
}

template<> float parseSingleItem<float>(const string &val) {
  bool foundperiod = false;
  for(int i = 0; i < val.size(); i++) {
    if(val[i] == '.') {
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
    arr[bucket] = atof(qoki[0].c_str());
  }
}

void parseHierarchy(kvData *chunk, bool reload) {
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

void parseLauncher(kvData *chunk, bool reload) {
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

void parseEffects(kvData *chunk, bool reload) {
  IDBEffects *titem = prepareName(chunk, &effectsclasses, reload, "effects");
  
  titem->quantity = atoi(chunk->consume("quantity").c_str());
  
  titem->inertia = atof(chunk->consume("inertia").c_str());
  titem->spread = atof(chunk->consume("spread").c_str());
  
  titem->slowdown = atof(chunk->consume("slowdown").c_str());
  titem->lifetime = atof(chunk->consume("lifetime").c_str());
  
  titem->radius = atof(chunk->consume("radius").c_str());
  titem->color = colorFromString(chunk->consume("color"));
}

void parseStats(kvData *chunk, bool reload) {
  IDBStats *titem = prepareName(chunk, &statsclasses, reload, "stats");
    
  titem->dps_efficiency = parseWithDefault(chunk, "dps_efficiency", 1.0);
  titem->cps_efficiency = parseWithDefault(chunk, "cps_efficiency", 1.0);
}

void parseWeapon(kvData *chunk, bool reload) {
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
    tnode.weapon = &weaponclasses[name];
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
}

void parseUpgrade(kvData *chunk, bool reload) {
  string name;
  IDBUpgrade *titem = prepareName(chunk, &upgradeclasses, reload, &name);
  
  titem->costmult = atoi(chunk->consume("costmult").c_str());
  
  titem->adjustment = parseSubclass(chunk->consume("adjustment"), adjustmentclasses);
  
  titem->text = parseOptionalSubclass(chunk, "text", text);
  
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
    tnode.upgrade = &upgradeclasses[name];
    tnode.spawncash = Money(titem->costmult * 1100);  // heh.
    mountpoint->branches.push_back(tnode);
  }
}

void parseProjectile(kvData *chunk, bool reload) {
  IDBProjectile *titem = prepareName(chunk, &projectileclasses, reload, "projectile");
  
  titem->thickness_visual = parseWithDefault(chunk, "thickness_visual", 0.3);
  
  string motion = parseWithDefault(chunk, "motion", "normal");
  if(motion == "normal") {
    titem->motion = PM_NORMAL;
  } else if(motion == "missile") {
    titem->motion = PM_MISSILE;
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

void parseDeploy(kvData *chunk, bool reload) {
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

void parseWarhead(kvData *chunk, bool reload) {
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

void parseGlory(kvData *chunk, bool reload) {
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

    tnode.glory = &gloryclasses[name];
    tnode.spawncash = titem->base_cost / 2;
    mountpoint->branches.push_back(tnode);
  }
}

void parseBombardment(kvData *chunk, bool reload) {
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
    tnode.bombardment = &bombardmentclasses[name];
    mountpoint->branches.push_back(tnode);
  }
}

void parseTank(kvData *chunk, bool reload) {
  string name;
  IDBTank *titem = prepareName(chunk, &tankclasses, reload, &name);
  
  string weapon = chunk->consume("weapon");
  CHECK(weaponclasses.count(weapon));
  
  titem->weapon = &weaponclasses[weapon];
  
  titem->health = atof(chunk->consume("health").c_str());
  titem->handling = atof(chunk->consume("handling").c_str());
  titem->engine = atof(chunk->consume("engine").c_str());
  titem->mass = atof(chunk->consume("mass").c_str());
  
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
    
    Coord2 centr = getCentroid(titem->vertices);
    for(int i = 0; i < titem->vertices.size(); i++)
      titem->vertices[i] -= centr;
    titem->firepoint -= centr;
    for(int i = 0; i < titem->minepath.size(); i++)
      titem->minepath[i] -= centr;
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
    tnode.tank = &tankclasses[name];
    mountpoint->branches.push_back(tnode);
  }
}

void parseAdjustment(kvData *chunk, bool reload) {
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

void parseFaction(kvData *chunk, bool reload) {
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

void parseText(kvData *chunk, bool reload) {
  string *titem = prepareName(chunk, &text, reload, "text");
  *titem = chunk->consume("data");
  // yay
}

void parseItemFile(const string &fname, bool reload) {
  ifstream tfil(fname.c_str());
  CHECK(tfil);
  kvData chunk;
  while(getkvData(tfil, &chunk)) {
    dprintf("%s\n", chunk.debugOutput().c_str());
    if(chunk.kv.count("debug") && atoi(chunk.consume("debug").c_str()) && !FLAGS_debugitems) {
      dprintf("Debug only, skipping\n");
      continue;
    }
    if(chunk.category == "hierarchy") {
      parseHierarchy(&chunk, reload);
    } else if(chunk.category == "weapon") {
      parseWeapon(&chunk, reload);
    } else if(chunk.category == "upgrade") {
      parseUpgrade(&chunk, reload);
    } else if(chunk.category == "projectile") {
      parseProjectile(&chunk, reload);
    } else if(chunk.category == "deploy") {
      parseDeploy(&chunk, reload);
    } else if(chunk.category == "warhead") {
      parseWarhead(&chunk, reload);
    } else if(chunk.category == "glory") {
      parseGlory(&chunk, reload);
    } else if(chunk.category == "bombardment") {
      parseBombardment(&chunk, reload);
    } else if(chunk.category == "tank") {
      parseTank(&chunk, reload);
    } else if(chunk.category == "adjustment") {
      parseAdjustment(&chunk, reload);
    } else if(chunk.category == "faction") {
      parseFaction(&chunk, reload);
    } else if(chunk.category == "text") {
      parseText(&chunk, reload);
    } else if(chunk.category == "launcher") {
      parseLauncher(&chunk, reload);
    } else if(chunk.category == "stats") {
      parseStats(&chunk, reload);
    } else if(chunk.category == "effects") {
      parseEffects(&chunk, reload);
    } else {
      CHECK(0);
    }
    if(!chunk.isDone()) {
      dprintf("Chunk still has unparsed data!\n");
      dprintf("%s\n", chunk.debugOutput().c_str());
      CHECK(0);
    }
  }
}

void loadItemDb(bool reload) {
  {
    CHECK(root.name == "");
    root.name = "ROOT";
    root.type = HierarchyNode::HNT_CATEGORY;
    root.displaymode = HierarchyNode::HNDM_BLANK;
  }
  
  string basepath = "data/base/";
  ifstream manifest((basepath + "manifest").c_str());
  string line;
  while(getLineStripped(manifest, &line)) {
    dprintf("%s\n", line.c_str());
    parseItemFile(basepath + line, reload);
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
  
  dprintf("done loading, consistency check\n");
  CHECK(deftank);
  CHECK(defglory);
  CHECK(defbombardment);
  root.checkConsistency();
  dprintf("Consistency check is awesome!\n");
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
  
  loadItemDb(true);
  
  CHECK(factions.size() == idfa.size());
  for(int i = 0; i < factions.size(); i++)
    CHECK(idfa[i] == &factions[i]);
}

void initItemdb() {
  CHECK(root.name == "");
  
  loadItemDb(false);
}

class ReloadHTTPD : public HTTPDhook {
public:

  string reply(const map<string, string> &params) {
    reloadItemdb();
    return "done";
  }

  ReloadHTTPD() : HTTPDhook("reload") { };
} reloadhttpd;

void generateCachedShops() {
  FILE *ofil = fopen("data/shopcache.dwh", "w");
  for(map<string, IDBWeapon>::const_iterator itr = weaponclasses.begin(); itr != weaponclasses.end(); itr++) {
    dprintf("%s\n", itr->first.c_str());
  }
  fclose(ofil);
}

void generateWeaponStats() {
  FILE *ofil = fopen("tools/weapondump.dat", "w");
  IDBAdjustment adj;
  map<string, vector<pair<float, float> > > goof;
  for(map<string, IDBWeapon>::const_iterator itr = weaponclasses.begin(); itr != weaponclasses.end(); itr++) {
    IDBWeaponAdjust wa(&itr->second, adj);
    string name = wa.name();
    name = string(name.c_str(), (const char*)strrchr(name.c_str(), ' '));
    if(wa.cost() > Money(0))
      goof[name].push_back(make_pair(wa.stats_damagePerSecond() * itr->second.launcher->stats->dps_efficiency, wa.stats_costPerSecond() * itr->second.launcher->stats->cps_efficiency));
  }
  
  for(map<string, vector<pair<float, float> > >::iterator itr = goof.begin(); itr != goof.end(); itr++) {
    sort(itr->second.begin(), itr->second.end());
    fprintf(ofil, "%s", itr->first.c_str());
    for(int i = 0; i < itr->second.size(); i++)
      fprintf(ofil, ",%f,%f", itr->second[i].first, itr->second[i].second);
    fprintf(ofil, "\n");
  }
  fclose(ofil);
}

void generateFactionStats() {
  FILE *ofil = fopen("tools/factiondump.dat", "w");
  CHECK(ofil);
  for(int j = 0; j < IDBAdjustment::LAST; j++)
    fprintf(ofil, "\t%s", adjust_text[j]);
  fprintf(ofil, "\n");
  for(int i = 0; i < factions.size(); i++) {
    fprintf(ofil, "%s", factions[i].name.c_str());
    for(int j = 0; j < IDBAdjustment::LAST; j++)
      fprintf(ofil, "\t%d", factions[i].adjustment[3]->adjusts[j]);
    fprintf(ofil, "\n");
  }
  fclose(ofil);
}

void dumpText() {
  FILE *ofil = fopen("tools/textdump.txt", "w");
  CHECK(ofil);
  for(map<string, string>::const_iterator itr = text.begin(); itr != text.end(); itr++) {
    fprintf(ofil, "%s\n\n\n\n", itr->second.c_str());
  }
  fclose(ofil);
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
const vector<IDBFaction> &factionList() {
  return factions;
}

