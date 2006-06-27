
#include "itemdb.h"

#include "args.h"
#include "const.h"
#include "parse.h"
#include "player.h"

#include <fstream>
#include <numeric>

using namespace std;

static HierarchyNode root;
static map<string, IDBDeploy> deployclasses;
static map<string, IDBWarhead> warheadclasses;
static map<string, IDBProjectile> projclasses;
static map<string, IDBWeapon> weaponclasses;
static map<string, IDBUpgrade> upgradeclasses;
static map<string, IDBGlory> gloryclasses;
static map<string, IDBBombardment> bombardmentclasses;
static map<string, IDBTank> tankclasses;
static map<string, IDBAdjustment> adjustmentclasses;
static vector<IDBFaction> factions;

static const IDBTank *deftank = NULL;
static const IDBGlory *defglory = NULL;
static const IDBBombardment *defbombardment = NULL;

DEFINE_bool(debugitems, false, "Enable debug items");

void IDBAdjustment::debugDump() {
  dprintf("IDBAdjustment debug dump");
  for(int i = 0; i < LAST; i++)
    if(adjusts[i])
      dprintf("%12s: %d", adjust_text[i], adjusts[i]);
}

IDBAdjustment::IDBAdjustment() {
  memset(adjusts, 0, sizeof(adjusts));
}

const IDBAdjustment &operator+=(IDBAdjustment &lhs, const IDBAdjustment &rhs) {
  for(int i = 0; i < IDBAdjustment::LAST; i++)
    lhs.adjusts[i] += rhs.adjusts[i];
  return lhs;
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
    return player->adjustUpgrade(upgrade).cost();
  } else if(type == HNT_GLORY) {
    return player->adjustGlory(glory).cost();
  } else if(type == HNT_BOMBARDMENT) {
    return player->adjustBombardment(bombardment).cost();
  } else if(type == HNT_TANK) {
    return player->adjustTank(tank).cost();
  } else {
    CHECK(0);
  }
}

Money HierarchyNode::sellvalue(const Player *player) const {
  if(type == HNT_WEAPON) {
    return player->adjustWeapon(weapon).sellcost(min(pack, player->ammoCount(weapon)));
  } else if(type == HNT_UPGRADE) {
    return player->adjustUpgrade(upgrade).sellcost();
  } else if(type == HNT_GLORY) {
    return player->adjustGlory(glory).sellcost();
  } else if(type == HNT_BOMBARDMENT) {
    return player->adjustBombardment(bombardment).sellcost();
  } else if(type == HNT_TANK) {
    return player->adjustTank(tank).sellcost();
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
    gottype = true;
    CHECK(displaymode != HNDM_COST);
    CHECK(!buyable);
  }
  
  // weapons all have costs and are buyable
  if(type == HNT_WEAPON) {
    gottype = true;
    CHECK(displaymode == HNDM_COST);
    CHECK(buyable);
    CHECK(weapon);
  } else {
    CHECK(!weapon);
  }
  
  // upgrades all are unique and are buyable
  if(type == HNT_UPGRADE) {
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
    gottype = true;
    CHECK(displaymode == HNDM_BLANK);
    CHECK(!buyable);
    CHECK(cat_restrictiontype == HNT_EQUIPWEAPON);
  }
  
  // the equipweapon item must have a weapon and is "buyable"
  if(type == HNT_EQUIPWEAPON) {
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
    gottype = true;
    CHECK(displaymode == HNDM_BLANK);
    CHECK(buyable);
    CHECK(pack == 1);
    CHECK(name == "done");
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

void parseItemFile(const string &fname) {
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
      HierarchyNode *mountpoint = findNamedNode(chunk.kv["name"], 1);
      HierarchyNode tnode;
      tnode.name = tokenize(chunk.consume("name"), ".").back();
      dprintf("name: %s\n", tnode.name.c_str());
      tnode.type = HierarchyNode::HNT_CATEGORY;
      if(chunk.kv.count("pack")) {
        tnode.displaymode = HierarchyNode::HNDM_PACK;
        tnode.pack = atoi(chunk.consume("pack").c_str());
        CHECK(mountpoint->pack == -1);
      } else {
        tnode.displaymode = HierarchyNode::HNDM_BLANK;
        tnode.pack = mountpoint->pack;
      }
      if(chunk.kv.count("type")) {
        if(chunk.kv["type"] == "weapon") {
          tnode.cat_restrictiontype = HierarchyNode::HNT_WEAPON;
        } else if(chunk.kv["type"] == "upgrade") {
          tnode.cat_restrictiontype = HierarchyNode::HNT_UPGRADE;
        } else if(chunk.kv["type"] == "glory") {
          tnode.cat_restrictiontype = HierarchyNode::HNT_GLORY;
        } else if(chunk.kv["type"] == "bombardment") {
          tnode.cat_restrictiontype = HierarchyNode::HNT_BOMBARDMENT;
        } else if(chunk.kv["type"] == "tank") {
          tnode.cat_restrictiontype = HierarchyNode::HNT_TANK;
        } else {
          dprintf("Unknown restriction type in hierarchy node: %s\n", chunk.kv["type"].c_str());
          CHECK(0);
        }
        chunk.consume("type");
      }
      if(tnode.cat_restrictiontype == -1) {
        tnode.cat_restrictiontype = mountpoint->cat_restrictiontype;
      }
      CHECK(mountpoint->cat_restrictiontype == -1 || tnode.cat_restrictiontype == mountpoint->cat_restrictiontype);
      mountpoint->branches.push_back(tnode);

    } else if(chunk.category == "weapon") {
      
      string name = chunk.consume("name");
      CHECK(weaponclasses.count(name) == 0);
      
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
        mountpoint->branches.push_back(tnode);
        
        CHECK(mountpoint->pack >= 1);
        weaponclasses[name].quantity = mountpoint->pack;
        weaponclasses[name].base_cost = moneyFromString(chunk.consume("cost"));
        CHECK(weaponclasses[name].base_cost > Money(0));
      } else {
        CHECK(!chunk.kv.count("cost"));
        weaponclasses[name].quantity = 100; // why not?
        weaponclasses[name].base_cost = Money(0);
      }
      
      weaponclasses[name].name = informal_name;
      weaponclasses[name].firerate = atof(chunk.consume("firerate").c_str());

      string projclass = chunk.consume("projectile");
      CHECK(projclasses.count(projclass));
      weaponclasses[name].projectile = &projclasses[projclass];

      string deployclass = chunk.consume("deploy");
      CHECK(deployclasses.count(deployclass));
      weaponclasses[name].deploy = &deployclasses[deployclass];
      
    } else if(chunk.category == "upgrade") {
      
      string name = chunk.consume("name");
      CHECK(upgradeclasses.count(name) == 0);
      
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
      mountpoint->branches.push_back(tnode);
      
      upgradeclasses[name].base_cost = moneyFromString(chunk.consume("cost"));
      string adjustment = chunk.consume("adjustment");
      CHECK(adjustmentclasses.count(adjustment));
      upgradeclasses[name].adjustment = &adjustmentclasses[adjustment];

    } else if(chunk.category == "projectile") {
      string name = chunk.consume("name");
      CHECK(prefixed(name, "projectile"));
      CHECK(projclasses.count(name) == 0);

      projclasses[name].motion = PM_NORMAL;
      projclasses[name].width = 0.1;
      
      projclasses[name].velocity = atof(chunk.consume("velocity").c_str()) / FPS;
      projclasses[name].color = colorFromString(chunk.consume("color"));
      
      if(chunk.kv.count("width")) {
        projclasses[name].width = atof(chunk.consume("width").c_str());
      }
      
      if(chunk.kv.count("motion")) {
        string motion = chunk.consume("motion");
        if(motion == "normal") {
          projclasses[name].motion = PM_NORMAL;
        } else if(motion == "missile") {
          projclasses[name].motion = PM_MISSILE;
        } else if(motion == "airbrake") {
          projclasses[name].motion = PM_AIRBRAKE;
        } else {
          dprintf("Unknown projectile motion: %s\n", motion.c_str());
          CHECK(0);
        }
      }
      
      string warheadclass = chunk.consume("warhead");
      CHECK(warheadclasses.count(warheadclass));
      projclasses[name].warhead = &warheadclasses[warheadclass];
      
    } else if(chunk.category == "deploy") {
      string name = chunk.consume("name");
      CHECK(prefixed(name, "deploy"));
      CHECK(deployclasses.count(name) == 0);
      
      deployclasses[name];
      
      deployclasses[name].anglestddev = 0;
      if(chunk.kv.count("anglestddev"))
        deployclasses[name].anglestddev = atof(chunk.consume("anglestddev").c_str());
      
    } else if(chunk.category == "warhead") {
      string name = chunk.consume("name");
      CHECK(prefixed(name, "warhead"));
      CHECK(warheadclasses.count(name) == 0);
      
      memset(warheadclasses[name].impactdamage, 0, sizeof(warheadclasses[name].impactdamage));
      memset(warheadclasses[name].radiusdamage, 0, sizeof(warheadclasses[name].radiusdamage));
      warheadclasses[name].radiusfalloff = -1;
      warheadclasses[name].wallremovalradius = 0;
      warheadclasses[name].wallremovalchance = 1;
      
      if(chunk.kv.count("impactdamage"))
        parseDamagecode(chunk.consume("impactdamage"), warheadclasses[name].impactdamage);
      
      CHECK(chunk.kv.count("radiusfalloff") == chunk.kv.count("radiusdamage"));
      
      if(chunk.kv.count("radiusdamage")) {
        parseDamagecode(chunk.consume("radiusdamage"), warheadclasses[name].radiusdamage);
        warheadclasses[name].radiusfalloff = atof(chunk.consume("radiusfalloff").c_str());
      }
      
      if(chunk.kv.count("wallremovalradius"))
        warheadclasses[name].wallremovalradius = atof(chunk.consume("wallremovalradius").c_str());
      
      if(chunk.kv.count("wallremovalchance"))
        warheadclasses[name].wallremovalchance = atof(chunk.consume("wallremovalchance").c_str());

    } else if(chunk.category == "glory") {
      
      string name = chunk.consume("name");
      CHECK(gloryclasses.count(name) == 0);
      
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
      mountpoint->branches.push_back(tnode);
      
      gloryclasses[name].base_cost = moneyFromString(chunk.consume("cost"));
      
      string projclass = chunk.consume("projectile");
      CHECK(projclasses.count(projclass));
      gloryclasses[name].projectile = &projclasses[projclass];

      string deployclass = chunk.consume("deploy");
      CHECK(deployclasses.count(deployclass));
      gloryclasses[name].deploy = &deployclasses[deployclass];

      gloryclasses[name].minsplits = atoi(chunk.consume("minsplits").c_str());
      gloryclasses[name].maxsplits = atoi(chunk.consume("maxsplits").c_str());
      gloryclasses[name].minsplitsize = atoi(chunk.consume("minsplitsize").c_str());
      gloryclasses[name].maxsplitsize = atoi(chunk.consume("maxsplitsize").c_str());
      gloryclasses[name].shotspersplit = atoi(chunk.consume("shotspersplit").c_str());
      
      if(chunk.kv.count("default") && atoi(chunk.consume("default").c_str())) {
        CHECK(!defglory);
        defglory = &gloryclasses[name];
      }
      
    } else if(chunk.category == "bombardment") {
      
      string name = chunk.consume("name");
      CHECK(bombardmentclasses.count(name) == 0);
      
      HierarchyNode *mountpoint = findNamedNode(name, 1);
      HierarchyNode tnode;
      tnode.name = tokenize(name, ".").back();
      tnode.type = HierarchyNode::HNT_BOMBARDMENT;
      tnode.displaymode = HierarchyNode::HNDM_COSTUNIQUE;
      tnode.buyable = true;
      tnode.pack = 1;
      tnode.cat_restrictiontype = HierarchyNode::HNT_BOMBARDMENT;
      CHECK(mountpoint->cat_restrictiontype == -1 || tnode.cat_restrictiontype == mountpoint->cat_restrictiontype);
      
      tnode.bombardment = &bombardmentclasses[name];
      mountpoint->branches.push_back(tnode);

      string warheadclass = chunk.consume("warhead");
      CHECK(warheadclasses.count(warheadclass));
      
      bombardmentclasses[name].base_cost = moneyFromString(chunk.consume("cost"));
      
      bombardmentclasses[name].warhead = &warheadclasses[warheadclass];

      bombardmentclasses[name].lockdelay = atoi(chunk.consume("lockdelay").c_str());
      bombardmentclasses[name].unlockdelay = atoi(chunk.consume("unlockdelay").c_str());

      if(chunk.kv.count("default") && atoi(chunk.consume("default").c_str())) {
        CHECK(!defbombardment);
        defbombardment = &bombardmentclasses[name];
      }
      
    } else if(chunk.category == "tank") {
      
      string name = chunk.consume("name");
      CHECK(tankclasses.count(name) == 0);
      
      HierarchyNode *mountpoint = findNamedNode(name, 1);
      HierarchyNode tnode;
      tnode.name = tokenize(name, ".").back();
      tnode.type = HierarchyNode::HNT_TANK;
      tnode.displaymode = HierarchyNode::HNDM_COSTUNIQUE;
      tnode.buyable = true;
      tnode.pack = 1;
      tnode.cat_restrictiontype = HierarchyNode::HNT_TANK;
      CHECK(mountpoint->cat_restrictiontype == -1 || tnode.cat_restrictiontype == mountpoint->cat_restrictiontype);
      
      tnode.tank = &tankclasses[name];
      mountpoint->branches.push_back(tnode);
      
      
      
      string weapon = chunk.consume("weapon");
      CHECK(weaponclasses.count(weapon));
      
      tankclasses[name].weapon = &weaponclasses[weapon];
      
      tankclasses[name].health = atof(chunk.consume("health").c_str());
      tankclasses[name].handling = atof(chunk.consume("handling").c_str());
      tankclasses[name].engine = atof(chunk.consume("engine").c_str());
      
      tankclasses[name].base_cost = moneyFromString(chunk.consume("cost"));
      
      if(chunk.kv.count("default") && atoi(chunk.consume("default").c_str())) {
        CHECK(!deftank);
        deftank = &tankclasses[name];
      }
      
    } else if(chunk.category == "adjustment") {
      
      string name = chunk.consume("name");
      CHECK(adjustmentclasses.count(name) == 0);
      
      adjustmentclasses[name];
      
      CHECK(sizeof(adjust_text) / sizeof(*adjust_text) == IDBAdjustment::LAST);
      
      for(int i = 0; i < IDBAdjustment::LAST; i++)
        if(chunk.kv.count(adjust_text[i]))
          adjustmentclasses[name].adjusts[i] = atoi(chunk.consume(adjust_text[i]).c_str());
      
    } else if(chunk.category == "faction") {
      
      IDBFaction fact;
      
      fact.icon = loadDvec2("data/base/faction_icons/" + chunk.consume("file"));
      fact.color = colorFromString(chunk.consume("color"));
      fact.name = chunk.consume("name");
      
      {
        vector<int> lines = sti(tokenize(chunk.consume("lines"), " "));
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
      
      string adjustment = chunk.consume("adjustment") +  ".high";
      CHECK(adjustmentclasses.count(adjustment));
      for(int i = 0; i < 3; i++)
        fact.adjustment[i] = &adjustmentclasses["null"]; // wheeeeeeeee
      fact.adjustment[3] = &adjustmentclasses[adjustment];
      
      factions.push_back(fact);
      
    } else if(chunk.category == "equip") {
      
      string name = chunk.consume("name");
      CHECK(bombardmentclasses.count(name) == 0);
      
      HierarchyNode *mountpoint = findNamedNode(name, 1);
      HierarchyNode tnode;
      tnode.name = tokenize(name, ".").back();
      tnode.type = HierarchyNode::HNT_EQUIP;
      tnode.displaymode = HierarchyNode::HNDM_BLANK;
      tnode.cat_restrictiontype = HierarchyNode::HNT_EQUIPWEAPON;
      CHECK(mountpoint->cat_restrictiontype == -1 || tnode.cat_restrictiontype == mountpoint->cat_restrictiontype);
      
      mountpoint->branches.push_back(tnode);
    
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

void initItemdb() {
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
    parseItemFile(basepath + line);
  }
  
  {
    // add our hardcoded "done" token
    HierarchyNode tnode;
    tnode.name = "done";
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

