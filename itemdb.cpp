
#include "itemdb.h"

#include "itemdb_parse.h"
#include "args.h"
#include "parse.h"
#include "httpd.h"

#include <fstream>
#include <numeric>
#include <set>

using namespace std;

HierarchyNode root;
map<string, IDBDeploy> deployclasses;
map<string, IDBWarhead> warheadclasses;
map<string, IDBProjectile> projectileclasses;
map<string, IDBLauncher> launcherclasses;
map<string, IDBWeapon> weaponclasses;
map<string, IDBUpgrade> upgradeclasses;
map<string, IDBGlory> gloryclasses;
map<string, IDBBombardment> bombardmentclasses;
map<string, IDBTank> tankclasses;
map<string, IDBAdjustment> adjustmentclasses;
map<string, IDBEffects> effectsclasses;
map<string, IDBImplantSlot> implantslotclasses;
map<string, IDBImplant> implantclasses;

vector<IDBFaction> factions;
map<string, string> text;

const IDBTank *deftank = NULL;
const IDBGlory *defglory = NULL;
const IDBBombardment *defbombardment = NULL;

map<string, IDBShopcache> shopcaches;

DEFINE_bool(shopcache, true, "Enable shop demo cache");

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

pair<string, bool> adjust_modifiertext(int id, int amount) {
  pair<string, bool> rv = make_pair("", amount > 0);
  if(id == IDBAdjustment::RECYCLE_BONUS) {
    IDBAdjustment idba;
    idba.adjusts[IDBAdjustment::RECYCLE_BONUS] = amount;
    rv.first = StringPrintf("%d%%", int(idba.recyclevalue() * 100));
  } else {
    rv.first = StringPrintf("%+d%%", amount);
  }
  return rv;
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
  tankhpboost = 0;
  tankspeedreduction = 0;
  ignore_excessive_radius = false;
}

IDBAdjustment operator*(const IDBAdjustment &lhs, int mult) {
  IDBAdjustment rv = lhs;
  for(int i = 0; i < IDBAdjustment::LAST; i++)
    rv.adjusts[i] *= mult;
  CHECK(!lhs.tankhpboost);
  CHECK(!lhs.tankspeedreduction);
  return rv;
}

const IDBAdjustment &operator+=(IDBAdjustment &lhs, const IDBAdjustment &rhs) {
  for(int i = 0; i < IDBAdjustment::LAST; i++)
    lhs.adjusts[i] += rhs.adjusts[i];
  CHECK(!lhs.tankhpboost || !rhs.tankhpboost);
  CHECK(!lhs.tankspeedreduction || !rhs.tankspeedreduction);
  lhs.tankhpboost += rhs.tankhpboost;
  lhs.tankspeedreduction += rhs.tankspeedreduction;
  return lhs;
}

bool operator==(const IDBAdjustment &lhs, const IDBAdjustment &rhs) {
  for(int i = 0; i < IDBAdjustment::LAST; i++)
    if(lhs.adjusts[i] != rhs.adjusts[i])
      return false;
  if(lhs.tankhpboost != rhs.tankhpboost)
    return false;
  if(lhs.tankspeedreduction != rhs.tankspeedreduction)
    return false;
  return true;
}

double IDBAdjustment::adjustmentfactor(int type) const {
  CHECK(type >= 0 && type < LAST);
  if(type == WARHEAD_RADIUS_FALLOFF) {
    if(!ignore_excessive_radius && adjusts[type] + 100 > WARHEAD_RADIUS_MAXMULT * 100) {
      dprintf("Adjust is %d vs %d\n", adjusts[type] + 100, WARHEAD_RADIUS_MAXMULT * 100);
      CHECK(adjusts[type] + 100 <= WARHEAD_RADIUS_MAXMULT * 100);
    }
  }
  if(adjust_negative[type]) {
    return 100. / (100. - adjusts[type]);
  } else {
    return (adjusts[type] + 100.) / 100.;
  }
}

double IDBAdjustment::recyclevalue() const {
  float shares = 1 * adjustmentfactor(IDBAdjustment::RECYCLE_BONUS);
  float ratio = shares / (shares + 1.0);
  return ratio;
}

double IDBAdjustment::tankhp() const {
  return (adjusts[TANK_ARMOR] + 100.) * (tankhpboost + 100.) / 10000.;
}

double IDBAdjustment::tankspeed() const {
  return (adjusts[TANK_SPEED] + 100.) * (100. - tankspeedreduction) / 10000.;
}

bool sortByTankCost(const HierarchyNode &lhs, const HierarchyNode &rhs) {
  CHECK(lhs.type == HierarchyNode::HNT_TANK);
  CHECK(rhs.type == HierarchyNode::HNT_TANK);
  return lhs.tank->base_cost > rhs.tank->base_cost;
}

void HierarchyNode::finalSort() {
  for(int i = 0; i < branches.size(); i++)
    branches[i].finalSort();
  
  if(type == HierarchyNode::HNT_CATEGORY && cat_restrictiontype == HierarchyNode::HNT_TANK)
    stable_sort(branches.begin(), branches.end(), sortByTankCost);
}

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
    CHECK(selectable);
    CHECK(!buyable);
  }
  
  // weapons all have costs and are buyable
  if(type == HNT_WEAPON) {
    CHECK(!gottype);
    gottype = true;
    CHECK(displaymode == HNDM_COST);
    CHECK(selectable);
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
    CHECK(selectable);
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
    CHECK(selectable);
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
    CHECK(selectable);
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
    CHECK(selectable);
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
    CHECK(selectable);
    CHECK(!buyable);
    CHECK(cat_restrictiontype == HNT_EQUIP_CAT);
  }
  
  // the sell item restricts on NONE and is not buyable
  if(type == HNT_SELL) {
    CHECK(!gottype);
    gottype = true;
    CHECK(displaymode == HNDM_BLANK);
    CHECK(selectable);
    CHECK(!buyable);
    CHECK(cat_restrictiontype == HNT_SELLWEAPON);
  }
  
  // the sellweapon item must have a weapon and is "buyable"
  if(type == HNT_SELLWEAPON) {
    CHECK(!gottype);
    gottype = true;
    CHECK(displaymode == HNDM_COST);
    CHECK(selectable);
    CHECK(buyable);
    CHECK(cat_restrictiontype == HNT_SELLWEAPON);
    CHECK(sellweapon);
  } else {
    CHECK(!sellweapon);
  }
  
  // the equipweapon item must have a weapon and may or may not be buyable
  if(type == HNT_EQUIPWEAPON) {
    CHECK(!gottype);
    gottype = true;
    CHECK(displaymode == HNDM_EQUIP);
    CHECK(selectable);
    CHECK(cat_restrictiontype == HNT_EQUIP_CAT);
    CHECK(equipweapon);
  } else {
    CHECK(!equipweapon);
    CHECK(!equipweaponfirst);
  }
  
  // the equipcategory item must have a category and isn't buyable
  if(type == HNT_EQUIPCATEGORY) {
    CHECK(!gottype);
    gottype = true;
    CHECK(displaymode == HNDM_BLANK);
    CHECK(!selectable);
    CHECK(!buyable);
    CHECK(cat_restrictiontype == HNT_EQUIP_CAT);
  }
  
  // implantslot must be buyable and have a slot of 1
  if(type == HNT_IMPLANTSLOT) {  
    CHECK(!gottype);
    gottype = true;
    CHECK(displaymode == HNDM_COSTUNIQUE);
    CHECK(selectable);
    CHECK(buyable);
    CHECK(pack == 1);
    CHECK(implantslot);
  } else {
    CHECK(!implantslot);
  }
  
  // implantitem must be buyable and have a slot of 1, but the rendering varies
  if(type == HNT_IMPLANTITEM || type == HNT_IMPLANTITEM_EQUIP || type == HNT_IMPLANTITEM_UPG) {  
    CHECK(!gottype);
    gottype = true;
    if(type == HNT_IMPLANTITEM) {
      CHECK(displaymode == HNDM_BLANK);
      CHECK(!selectable);
      CHECK(!buyable);
    } else if(type == HNT_IMPLANTITEM_EQUIP) {
      CHECK(displaymode == HNDM_IMPLANT_EQUIP);
      CHECK(selectable);
      CHECK(buyable);
      CHECK(pack == 1);
    } else if(type == HNT_IMPLANTITEM_UPG) {
      CHECK(displaymode == HNDM_IMPLANT_UPGRADE);
      CHECK(selectable);
      CHECK(buyable);
      CHECK(pack == 1);
    } else {
      CHECK(0);
    }
    CHECK(implantitem);
  } else {
    CHECK(!implantitem);
  }
  
  // the "bonuses" token has no cost or other display but is "buyable"
  if(type == HNT_BONUSES) {
    CHECK(!gottype);
    gottype = true;
    CHECK(displaymode == HNDM_BLANK);
    CHECK(selectable);
    CHECK(!buyable);
  }

  // the "done" token has no cost or other display but is "buyable"
  if(type == HNT_DONE) {
    CHECK(!gottype);
    gottype = true;
    CHECK(displaymode == HNDM_BLANK);
    CHECK(selectable);
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
  
  // if it's not a category, an equip, or a sell, it shouldn't have branches
  CHECK(type == HNT_CATEGORY || type == HNT_EQUIP || type == HNT_SELL || branches.size() == 0);
  
  // last, check the consistency of everything recursively
  for(int i = 0; i < branches.size(); i++) {
    if(cat_restrictiontype == HNT_IMPLANT_CAT) {
      CHECK(branches[i].type == HNT_IMPLANTSLOT || branches[i].type == HNT_IMPLANTITEM || branches[i].type == HNT_IMPLANTITEM_UPG || branches[i].type == HNT_IMPLANTITEM_EQUIP);
      CHECK(branches[i].cat_restrictiontype == cat_restrictiontype);
    } else if(cat_restrictiontype == HNT_EQUIP_CAT) {
      CHECK(branches[i].type == HNT_EQUIPWEAPON || branches[i].type == HNT_EQUIPCATEGORY);
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

const Color nhcolor[] = { Color(0.7, 0.7, 0.8), Color(0.5, 0.5, 1.0), Color(0.9, 0.5, 0.2), Color(0.2, 0.7, 0.7), Color(0.4, 0.7, 0.2) };

template<typename T> class getDamageType;

template<> class getDamageType<IDBWeapon> {
public:
  float operator()(const IDBWeapon *item, int type) {
    return IDBWeaponAdjust(item, IDBAdjustment()).stats_damagePerSecondType(type);
  }
};

template<> class getDamageType<IDBBombardment> {
public:
  float operator()(const IDBBombardment *item, int type) {
    return IDBBombardmentAdjust(item, IDBAdjustment(), 0).stats_damagePerShotType(type);
  }
};
  
template<typename T> Color gcolor(const T *item) {
  int dmg = -1;
  for(int i = 0; i < IDBAdjustment::DAMAGE_LAST; i++) {
    if(getDamageType<T>()(item, i) != 0) {
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
}

Color HierarchyNode::getColor() const {
  if(type == HNT_WEAPON) {
    return gcolor(weapon);
  } else if(type == HNT_BOMBARDMENT) {
    return gcolor(bombardment);
  } else if(type == HNT_EQUIPWEAPON) {
    Color col = gcolor(equipweapon);
    if(equipweaponfirst)
      col *= 1.5;
    return col;
  } else if(type == HNT_SELLWEAPON) {
    return gcolor(sellweapon);
  } else if(type == HNT_CATEGORY && branches.size()) {
    if(name == "Bombardment")
      return C::inactive_text; // hackety hack hack
    Color col = branches[0].getColor();
    for(int i = 1; i < branches.size(); i++)
      if(branches[i].getColor() != col)
        return C::inactive_text;
    return col;
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
  selectable = true;
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
  equipweaponfirst = false;
  sellweapon = NULL;
  spawncash = Money(0);
  despawncash = Money(0);
}

void swap(HierarchyNode &lhs, HierarchyNode &rhs) {
  if(&lhs == &rhs)
    return;
  vector<HierarchyNode> lb;
  vector<HierarchyNode> rb;
  lb.swap(lhs.branches);
  rb.swap(rhs.branches);
  HierarchyNode temp = lhs;
  lhs = rhs;
  rhs = temp;
  lb.swap(rhs.branches);
  rb.swap(lhs.branches);
}

void clearItemdb() {
  root = HierarchyNode();
  
  deployclasses.clear();
  warheadclasses.clear();
  projectileclasses.clear();
  launcherclasses.clear();
  weaponclasses.clear();
  upgradeclasses.clear();
  gloryclasses.clear();
  bombardmentclasses.clear();
  tankclasses.clear();
  adjustmentclasses.clear();
  effectsclasses.clear();
  implantslotclasses.clear();
  implantclasses.clear();
  factions.clear();
  text.clear();
  shopcaches.clear();

  deftank = NULL;
  defglory = NULL;
  defbombardment = NULL;
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
    tnode.name = "Sell ammo";
    tnode.type = HierarchyNode::HNT_SELL;
    tnode.displaymode = HierarchyNode::HNDM_BLANK;
    tnode.cat_restrictiontype = HierarchyNode::HNT_SELLWEAPON;
    root.branches.push_back(tnode);
  }
  
  // add our hardcoded "equip" token
  {
    HierarchyNode tnode;
    tnode.name = "Equip weapons";
    tnode.type = HierarchyNode::HNT_EQUIP;
    tnode.displaymode = HierarchyNode::HNDM_BLANK;
    tnode.cat_restrictiontype = HierarchyNode::HNT_EQUIP_CAT;
    root.branches.push_back(tnode);
  }
  
    // bonuses bonuses bonuses! malkovich bonuses?
  {
    HierarchyNode tnode;
    tnode.name = "Show bonuses";
    tnode.type = HierarchyNode::HNT_BONUSES;
    tnode.displaymode = HierarchyNode::HNDM_BLANK;
    tnode.cat_restrictiontype = HierarchyNode::HNT_BONUSES;
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
    parseShopcacheFile("data/shopcache.dwh", &errors);
  }
  
  dprintf("done loading, consistency check\n");
  CHECK(deftank);
  CHECK(defglory);
  CHECK(defbombardment);
  root.checkConsistency(&errors);
  
  dprintf("sorting/processing\n");
  root.finalSort();
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

void loadItemdb() {
  CHECK(root.name == "");
  
  loadItemDb(false);
}

IDBAdjustment IDBImplant::makeAdjustment(int level) const {
  CHECK(level > 0);
  return *adjustment * level;
}

vector<Coord2> IDBTank::getTankVertices(Coord2 pos, Coord td) const {
  Coord2 xt = makeAngle(td);
  Coord2 yt = makeAngle(td - COORDPI / 2);
  vector<Coord2> rv(vertices.size());
  for(int i = 0; i < vertices.size(); i++) {  // this bit of weirdness brought to you by CPU efficiency, since this function is called surprisingly often
    rv[i].x = pos.x + vertices[i].x * xt.x + vertices[i].y * xt.y;
    rv[i].y = pos.y + vertices[i].y * yt.y + vertices[i].x * yt.x;
  }
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

const map<string, IDBWeapon> &weaponList() { return weaponclasses; }
const map<string, IDBBombardment> &bombardmentList() { return bombardmentclasses; }
const map<string, IDBGlory> &gloryList() { return gloryclasses; }
const map<string, IDBTank> &tankList() { return tankclasses; }
const map<string, IDBUpgrade> &upgradeList() { return upgradeclasses; }
const vector<IDBFaction> &factionList() { return factions; }
const map<string, string> &textList() { return text; }

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

static map<const IDBTank *, string> tankclasses_reverse;
const string &nameFromIDB(const IDBTank *idbw) {
  return nameFromIDBImp(idbw, tankclasses, &tankclasses_reverse);
}

string informalNameFromIDB(const IDBWeapon *idbw) {
  return tokenize(nameFromIDB(idbw), ".").back();
}
string informalNameFromIDB(const IDBWeaponAdjust &idbwa) {
  return informalNameFromIDB(idbwa.base());
}
