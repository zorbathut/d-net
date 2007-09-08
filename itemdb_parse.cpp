
#include "itemdb_parse.h"

#include "itemdb_private.h"
#include "parse.h"
#include "regex.h"
#include "args.h"

#include <set>
#include <numeric>
#include <fstream>

using namespace std;

DEFINE_bool(debugitems, false, "Enable debug items");

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
  CHECK(val.size());
  for(int i = 0; i < val.size(); i++)
    CHECK(isdigit(val[i]));
  return atoi(val.c_str());
}

template<> float parseSingleItem<float>(const string &val) {
  CHECK(val.size());
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

template<> Color parseSingleItem<Color>(const string &val) {
  return colorFromString(val);
}

template<> Money parseSingleItem<Money>(const string &val) {
  return moneyFromString(val);
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
      dprintf("Parent node %s not found for item hierarchy!\n", in.c_str());
    }
    CHECK(fc == 1);
    CHECK(fi != -1);
    current = &current->branches[fi];
  }
  return current;
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
  tnode.despawncash = parseWithDefault(chunk, "despawncash", Money(0));
  
  CHECK(mountpoint->cat_restrictiontype == -1 || tnode.cat_restrictiontype == mountpoint->cat_restrictiontype);
  mountpoint->branches.push_back(tnode);
}

void parseLauncher(kvData *chunk, bool reload, ErrorAccumulator &accum) {
  IDBLauncher *titem = prepareName(chunk, &launcherclasses, reload, "launcher");
  
  titem->deploy = parseSubclass(chunk->consume("deploy"), deployclasses);

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
  } else if(demotype == "back") {
    titem->demomode = WDM_BACKRANGE;
  } else if(demotype == "mines") {
    titem->demomode = WDM_MINES;
  } else {
    CHECK(0);
  }
}

void parseEffects(kvData *chunk, bool reload, ErrorAccumulator &accum) {
  IDBEffects *titem = prepareName(chunk, &effectsclasses, reload, "effects");
  
  string effecttype = chunk->consume("type");
  
  if(effecttype == "particle") {
    titem->type = IDBEffects::EFT_PARTICLE;
    
    titem->particle_distribute = parseWithDefault(chunk, "distribute", false);
    
    titem->particle_multiple_inertia = parseWithDefault(chunk, "multiple_inertia", 0.f);
    titem->particle_multiple_reflect = parseWithDefault(chunk, "multiple_reflect", 0.f);
    titem->particle_multiple_force = parseWithDefault(chunk, "multiple_force", 0.f);
    
    titem->particle_spread = parseWithDefault(chunk, "spread", 0.f);
    
    titem->particle_slowdown = parseWithDefault(chunk, "slowdown", 1.f);
    titem->particle_lifetime = parseSingleItem<float>(chunk->consume("lifetime"));
    
    titem->particle_radius = parseSingleItem<float>(chunk->consume("radius"));
    titem->particle_color = parseSingleItem<Color>(chunk->consume("color"));
  } else if(effecttype == "ionblast") {
    titem->type = IDBEffects::EFT_IONBLAST;
    
    titem->ionblast_radius = parseSingleItem<float>(chunk->consume("radius"));
    titem->ionblast_duration = parseSingleItem<float>(chunk->consume("duration"));
    
    vector<string> vislist = tokenize(chunk->consume("visuals"), "\n");
    for(int i = 0; i < vislist.size(); i++) {
      boost::smatch mch = match(vislist[i], "([0-9]+) (.*)");
      titem->ionblast_visuals.push_back(make_pair(parseSingleItem<int>(mch[1]), parseSingleItem<Color>(mch[2])));
    }
    dprintf("done\n");
  } else {
    CHECK(0);
  }
  
  titem->quantity = parseWithDefault(chunk, "quantity", 1);
  
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
  
  titem->firerate = atof(chunk->consume("firerate").c_str());
  
  titem->launcher = parseSubclass(chunk->consume("launcher"), launcherclasses);
  
  titem->recommended = parseWithDefault(chunk, "recommended", -1);
  titem->glory_resistance = parseWithDefault(chunk, "glory_resistance", false);
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
  
  titem->costmult = strtod(chunk->consume("costmult").c_str(), NULL);
  
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
  
  titem->visual_thickness = parseWithDefault(chunk, "visual_thickness", 0.5);
  
  set<string> allowed_shapes;
  
  titem->proximity_visibility = -1;
  titem->halflife = parseWithDefault(chunk, "halflife", -1.);
  titem->penetrating = parseWithDefault(chunk, "penetrating", false);
  
  string motion = parseWithDefault(chunk, "motion", "normal");
  string defshape = "line";
  if(motion == "normal") {
    titem->motion = PM_NORMAL;
    allowed_shapes.insert("line");
    allowed_shapes.insert("arrow");
  } else if(motion == "missile") {
    titem->motion = PM_MISSILE;
    titem->missile_stabstart = parseSingleItem<float>(chunk->consume("missile_stabstart"));
    titem->missile_stabilization = parseSingleItem<float>(chunk->consume("missile_stabilization"));
    titem->missile_sidelaunch = parseSingleItem<float>(chunk->consume("missile_sidelaunch"));
    titem->missile_backlaunch = parseSingleItem<float>(chunk->consume("missile_backlaunch"));
    allowed_shapes.insert("line");
    allowed_shapes.insert("drone");
    allowed_shapes.insert("arrow");
  } else if(motion == "airbrake") {
    titem->motion = PM_AIRBRAKE;
    titem->airbrake_life = parseSingleItem<float>(chunk->consume("airbrake_life"));
    titem->airbrake_slowdown = parseSingleItem<float>(chunk->consume("airbrake_slowdown"));
    defshape = "line_airbrake";
    allowed_shapes.insert("line");
    allowed_shapes.insert("line_airbrake");
    allowed_shapes.insert("arrow");
  } else if(motion == "boomerang") {
    titem->motion = PM_BOOMERANG;
    titem->boomerang_convergence = parseSingleItem<float>(chunk->consume("boomerang_convergence"));
    titem->boomerang_intersection = parseSingleItem<float>(chunk->consume("boomerang_intersection"));
    titem->boomerang_maxrotate = parseSingleItem<float>(chunk->consume("boomerang_maxrotate"));
    allowed_shapes.insert("arrow");
  } else if(motion == "mine") {
    titem->motion = PM_MINE;
    titem->proximity_visibility = parseWithDefault(chunk, "proximity_visibility", 30.f);
    CHECK(titem->halflife != -1);
    allowed_shapes.insert("star");
  } else if(motion == "spidermine") {
    titem->motion = PM_SPIDERMINE;
    titem->proximity_visibility = parseWithDefault(chunk, "proximity_visibility", 30.f);
    CHECK(titem->halflife != -1);
    allowed_shapes.insert("star");
  } else if(motion == "hunter") {
    titem->motion = PM_HUNTER;
    titem->hunter_rotation = parseSingleItem<float>(chunk->consume("hunter_rotation"));
    titem->hunter_turnweight = parseSingleItem<float>(chunk->consume("hunter_turnweight"));
    allowed_shapes.insert("drone");
  } else if(motion == "dps") {
    titem->motion = PM_DPS;
    titem->dps_duration = parseSingleItem<float>(chunk->consume("duration"));
    titem->dps_instant_warhead = parseSubclassSet(chunk, "dps_instant_warhead", warheadclasses);
    defshape = "invisible";
    allowed_shapes.insert("invisible");
  } else {
    dprintf("Unknown projectile motion: %s\n", motion.c_str());
    CHECK(0);
  }
  
  titem->velocity = 0;
  if(titem->motion != PM_MINE && titem->motion != PM_DPS)
    titem->velocity = parseSingleItem<float>(chunk->consume("velocity"));
  
  bool has_color = true;
  
  string shape = parseWithDefault(chunk, "shape", defshape);
  CHECK(allowed_shapes.count(shape));
  if(shape == "line") {
    titem->shape = PS_LINE;
    titem->line_length = parseWithDefault(chunk, "line_length", titem->velocity / 60);  // yes, this is 60, not FPS
  } else if(shape == "line_airbrake") {
    titem->shape = PS_LINE_AIRBRAKE;
  } else if(shape == "arrow") {
    titem->shape = PS_ARROW;
    titem->arrow_height = parseSingleItem<float>(chunk->consume("arrow_height"));
    titem->arrow_width = parseSingleItem<float>(chunk->consume("arrow_width"));
    titem->arrow_rotate = parseWithDefault(chunk, "arrow_rotate", 0.0);
  } else if(shape == "star") {
    titem->shape = PS_STAR;
    titem->star_radius = atof(chunk->consume("star_radius").c_str());
    titem->star_spikes = parseSingleItem<int>(chunk->consume("star_spikes"));
  } else if(shape == "drone") {
    titem->shape = PS_DRONE;
    titem->drone_radius = parseSingleItem<float>(chunk->consume("drone_radius"));
    titem->drone_spike = parseSingleItem<float>(chunk->consume("drone_spike"));
  } else if(shape == "invisible") {
    titem->shape = PS_INVISIBLE;
    has_color = false;
  } else {
    CHECK(0);
  }
  
  if(has_color)
    titem->color = parseWithDefault(chunk, "color", C::gray(1.0));
  
  titem->chain_warhead = parseSubclassSet(chunk, "warhead", warheadclasses);
  titem->chain_deploy = parseSubclassSet(chunk, "deploy", deployclasses);
  titem->chain_effects = parseSubclassSet(chunk, "effects", effectsclasses);
  
  titem->burn_effects = parseSubclassSet(chunk, "burn_effects", effectsclasses);
  
  {
    string prox = parseWithDefault(chunk, "proximity", "off");
    if(prox == "off") {
      titem->proximity = -1;
    } else if(prox == "auto") {
      float rad = -1;
      for(int i = 0; i < titem->chain_warhead.size(); i++)
        rad = max(rad, titem->chain_warhead[i]->radiusfalloff);
      CHECK(rad > 0);
      titem->proximity = rad;
    } else {
      titem->proximity = parseSingleItem<float>(prox);
    }
  }
  
  if(titem->motion != PM_MINE && titem->motion != PM_DPS && titem->motion != PM_SPIDERMINE) {
    titem->no_intersection = parseWithDefault(chunk, "no_intersection", false);
    if(!titem->no_intersection)
      titem->durability = parseSingleItem<float>(chunk->consume("durability"));
    else
      titem->durability = -1;
  } else {
    titem->no_intersection = true;
    titem->durability = -1;
  }
  
  CHECK(titem->chain_warhead.size() || titem->chain_deploy.size());
}

void parseDeploy(kvData *chunk, bool reload, ErrorAccumulator &accum) {
  IDBDeploy *titem = prepareName(chunk, &deployclasses, reload, "deploy");
  
  string type = parseWithDefault(chunk, "type", "normal");
  
  if(type == "normal") {
    titem->type = DT_NORMAL;
  } else if(type == "forward") {
    titem->type = DT_FORWARD;
  } else if(type == "rear") {
    titem->type = DT_REAR;
  } else if(type == "centroid") {
    titem->type = DT_CENTROID;
  } else if(type == "minepath") {
    titem->type = DT_MINEPATH;
  } else if(type == "directed") {
    titem->type = DT_DIRECTED;
    titem->directed_range = parseSingleItem<float>(chunk->consume("directed_range"));
  } else if(type == "explode") {
    titem->type = DT_EXPLODE;
    titem->exp_minsplits = parseSingleItem<int>(chunk->consume("exp_minsplits"));
    titem->exp_maxsplits = parseSingleItem<int>(chunk->consume("exp_maxsplits"));
    titem->exp_minsplitsize = parseSingleItem<int>(chunk->consume("exp_minsplitsize"));
    titem->exp_maxsplitsize = parseSingleItem<int>(chunk->consume("exp_maxsplitsize"));
    titem->exp_shotspersplit = parseSingleItem<int>(chunk->consume("exp_shotspersplit"));
  } else {
    CHECK(0);
  }
  
  titem->anglestddev = parseWithDefault(chunk, "anglestddev", 0.0);
  titem->anglemodifier = parseWithDefault(chunk, "anglemodifier", 0.0);
  
  titem->chain_deploy = parseSubclassSet(chunk, "deploy", deployclasses);
  titem->chain_projectile = parseSubclassSet(chunk, "projectile", projectileclasses);
  titem->chain_warhead = parseSubclassSet(chunk, "warhead", warheadclasses);
  titem->chain_effects = parseSubclassSet(chunk, "effects", effectsclasses);
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
  CHECK(chunk->kv.count("radiusfalloff") == chunk->kv.count("radiusexplosive"));
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
  titem->radiusexplosive = parseWithDefault(chunk, "radiusexplosive", -1.0);
  if(titem->radiusfalloff != -1)
    CHECK(titem->radiusexplosive >= 0 && titem->radiusexplosive <= 1);
  
  
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
  
  titem->warheads = parseSubclassSet(chunk, "warhead", warheadclasses);
  titem->projectiles = parseSubclassSet(chunk, "projectile", projectileclasses);
  titem->effects = parseSubclassSet(chunk, "effects", effectsclasses);
  
  titem->showdirection = parseWithDefault(chunk, "showdirection", false);
  
  titem->cost = moneyFromString(chunk->consume("cost"));

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
    
    tnode.spawncash = titem->cost / 2;
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
    bool got_rearfirepoint = false;
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
        } else if(vti[2] == "rear_firepoint") {
          CHECK(!got_rearfirepoint);
          CHECK(in_mine_path);
          titem->rearfirepoint = this_vertex;
          got_rearfirepoint = true;
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
    
    CHECK(got_firepoint);
    CHECK(got_rearfirepoint);
    
    titem->centering_adjustment = getCentroid(titem->vertices);
    for(int i = 0; i < titem->vertices.size(); i++)
      titem->vertices[i] -= titem->centering_adjustment;
    titem->firepoint -= titem->centering_adjustment;
    titem->rearfirepoint -= titem->centering_adjustment;
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
  fact.adjustment[3] = parseSubclass(chunk->consume("adjustment"), adjustmentclasses);
  
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
    CHECK(tis.size() % 2 == 0);
    entry.count = atoi(tis[0].c_str());
    entry.warhead = &warheadclasses[tis[1]];
    entry.mult = atof(tis[2].c_str());
    if(tis[3] == "none") {
      entry.impact = -1;
    } else {
      entry.impact = atoi(tis[3].c_str());
    }
    for(int i = 4; i < tis.size(); i += 2) {
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

void parseShopcacheFile(const string &fname, vector<string> *errors) {
  ifstream shopcache(fname.c_str());
  if(shopcache) {
    dprintf("Loading shop cache");
    kvData chunk;
    while(getkvData(shopcache, &chunk)) {
      CHECK(chunk.category == "shopcache");
      parseShopcache(&chunk, errors);
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
  
  // this is kind of grim - we push three nodes in. This could happen at shop manipulation time also, I suppose.
  {
    HierarchyNode *mountpoint = findNamedNode(name, 1);
    HierarchyNode tnode;
    tnode.name = tokenize(name, ".").back();
    tnode.type = HierarchyNode::HNT_IMPLANTITEM;
    tnode.displaymode = HierarchyNode::HNDM_BLANK;
    tnode.buyable = false;
    tnode.selectable = false;
    tnode.cat_restrictiontype = HierarchyNode::HNT_IMPLANT_CAT;
    CHECK(mountpoint->cat_restrictiontype == -1 || tnode.cat_restrictiontype == mountpoint->cat_restrictiontype);
    
    tnode.implantitem = titem;
    mountpoint->branches.push_back(tnode);
  }
  
  {
    HierarchyNode *mountpoint = findNamedNode(name, 1);
    HierarchyNode tnode;
    tnode.name = tokenize(name, ".").back();
    tnode.type = HierarchyNode::HNT_IMPLANTITEM_EQUIP;
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

kvData currentlyreading;
void printCurread() {
  dprintf("%s\n", stringFromKvData(currentlyreading).c_str());
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
    currentlyreading = chunk;
    registerCrashFunction(printCurread);
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
    } else if(chunk.category == "effects") {
      parseEffects(&chunk, reload, erac);
    } else if(chunk.category == "implantslot") {
      parseImplantSlot(&chunk, reload, erac);
    } else if(chunk.category == "implant") {
      parseImplant(&chunk, reload, erac);
    } else {
      dprintf("Confusing category. Are you insane?\n");
      dprintf("%s\n", stringFromKvData(chunk).c_str());
      CHECK(0);
    }
    unregisterCrashFunction(printCurread);
    if(!chunk.isDone())
      erac.addError(StringPrintf("Chunk still has unparsed data! %s", chunk.debugOutput().c_str()));
  }
}
