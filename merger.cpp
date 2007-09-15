
#include "os.h"
#include "args.h"
#include "debug.h"
#include "merger_weapon.h"
#include "merger_bombardment.h"
#include "merger_tanks.h"
#include "merger_glory.h"
#include "merger_upgrades.h"
#include "merger_util.h"
#include "merger_factions.h"

#include <fstream>
#include <string>
#include <map>

using namespace std;

template<typename Model, bool twopass> struct PAW;

template<typename Model> struct PAW<Model, false> {
  static void f(const map<string, typename Model::Data> &tdd, const vector<kvData> &preproc, const string &merged) {
    StackString ss("PAWfalse");
    ofstream ofs(merged.c_str());
    for(int i = 0; i < preproc.size(); i++) {
      checkForExtraMerges(preproc[i]);
      ofs << stringFromKvData(preproc[i]);
    }
  }
};

template<typename Model> struct PAW<Model, true> {
  static void f(const map<string, typename Model::Data> &tdd, const vector<kvData> &preproc, const string &merged) {
    StackString ss("PAWtrue");
    set<string> names;
    for(typename map<string, typename Model::Data>::const_iterator itr = tdd.begin(); itr != tdd.end(); itr++)
      names.insert(itr->first);
    {
      ofstream ofs(merged.c_str());
      for(int i = 0; i < preproc.size(); i++) {
        kvData kvd = preproc[i];
        Model::testprocess(&kvd);
        checkForExtraMerges(kvd);
        ofs << stringFromKvData(kvd);
      }
    }
    
    addItemFile("data/base/hierarchy.dwh");
    vector<string> deps = Model::dependencies();
    for(int i = 0; i < deps.size(); i++)
      addItemFile(deps[i]);
    addItemFile(merged);
    
    map<string, float> multipliers;
    for(typename map<string, typename Model::FinalType>::const_iterator itr = Model::finalTypeList().begin(); itr != Model::finalTypeList().end(); itr++) {
      string name = Model::nameFromKvname(itr->first, names);
      if(!name.size())
          continue;
      CHECK(tdd.count(name));
      CHECK(!multipliers.count(name));
      multipliers[name] = Model::getMultiple(itr->second, tdd.find(name)->second);
      string mn = Model::getMultipleAltName(itr->first);
      if(mn.size()) {
        if(multipliers.count(mn))
          CHECK(withinEpsilon(multipliers[mn], Model::getMultiple(itr->second, tdd.find(name)->second), 0.00001));
        else {
          multipliers[mn] = Model::getMultiple(itr->second, tdd.find(name)->second);
          names.insert(mn);
        }
      }
    }
    
    clearItemdb();
    
    {
      ofstream ofs(merged.c_str());
      for(int i = 0; i < preproc.size(); i++) {
        string name = Model::nameFromKvname(preproc[i].read("name"), names);
        kvData kvd = preproc[i];
        if(multipliers.count(name)) {
          CHECK(multipliers.count(name));
          Model::reprocess(&kvd, multipliers[name]);
        }
        checkForExtraMerges(kvd);
        ofs << stringFromKvData(kvd);
      }
    }
  }
};

template<typename Model> void processAndWrite(const map<string, typename Model::Data> &tdd, const vector<kvData> &preproc, const string &merged) {
  PAW<Model, Model::twopass>::f(tdd, preproc, merged);
}

template<typename Model, bool direct> struct NFK;

template<typename Model> struct NFK<Model, false> {
  static string f(const kvData &kvd, const set<string> &possiblenames) {
    return Model::nameFromKvname(kvd.read("name"), possiblenames);
  }
};
template<typename Model> struct NFK<Model, true> {
  static string f(const kvData &kvd, const set<string> &possiblenames) {
    return Model::nameFromKvd(kvd, possiblenames);
  }
};

template<typename Model> string nameFromKvd(const kvData &kvd, const set<string> &possiblenames) {
  return NFK<Model, Model::kvdirect>::f(kvd, possiblenames);
}

template<typename Model> void doMerge(const string &csv, const string &unmerged, const string &merged) {
  StackString ss("domerge");
  map<string, typename Model::Data> tdd;
  set<string> names;
  {
    ifstream ifs(csv.c_str());
    
    typename Model::Namer namer;
    
    string lin;
    while(getline(ifs, lin)) {
      vector<string> dt = parseCsv(lin);
      
      if(dt[0] == Model::token())
        continue;
      
      string name = namer.getName(dt);
      if(!name.size())
        continue;
      
      CHECK(!tdd.count(name));
      
      //dprintf("pl \"%s\"\n", name.c_str());
      typename Model::Data dat;
      if(!Model::parseLine(dt, &dat))
        continue;
      
      //dprintf("got name \"%s\"\n", name.c_str());
      tdd[name] = dat;
      names.insert(name);
    }
  }
  
//  dprintf("Got %d items\n", tdd.size());
  
  vector<kvData> preproc;
  {
    set<string> done;
    ifstream ifs(unmerged.c_str());
    kvData kvd;
    while(getkvData(ifs, &kvd)) {
      string name = nameFromKvd<Model>(kvd, names);
      //dprintf("Name is \"%s\", checking \"%s\"\n", kvd.read("name").c_str(), name.c_str());
      if(name.size()) {
        CHECK(tdd.count(name));
        done.insert(name);
        Model::preprocess(&kvd, tdd[name]);
      }
      preproc.push_back(kvd);
    }
    
    CHECK(done.size() == tdd.size());
    for(typename map<string, typename Model::Data>::const_iterator itr = tdd.begin(); itr != tdd.end(); itr++)
      CHECK(done.count(itr->first));
  }
  
  processAndWrite<Model>(tdd, preproc, merged);
  
  addItemFile("data/base/hierarchy.dwh");
  vector<string> deps = Model::dependencies();
    for(int i = 0; i < deps.size(); i++)
      addItemFile(deps[i]);
  addItemFile(merged);
  
  for(typename map<string, typename Model::FinalType>::const_iterator itr = Model::finalTypeList().begin(); itr != Model::finalTypeList().end(); itr++) {
    string tt;
    if(tdd.count(suffix(itr->first))) {
      tt = suffix(itr->first);
    } else {
      tt = itr->first;
    }
    CHECK(tdd.count(tt));
    Model::verify(itr->second, tdd[tt]);
    tdd.erase(tt);
  }
}  

int main(int argc, char *argv[]) {
  set_exename("merger.exe");
  initFlags(argc, argv, 3);
  
  StackString ss("core");
  
  CHECK(argc == 4);
  
  string type;
  {
    ifstream ifs(argv[1]);
    string line;
    CHECK(getline(ifs, line));
    type = parseCsv(line)[0];
  }
  //dprintf("Got type %s\n", type.c_str());
  
  if(type == WeaponParams::token()) {
    doMerge<WeaponParams>(argv[1], argv[2], argv[3]);
  } else if(type == BombardParams::token()) {
    doMerge<BombardParams>(argv[1], argv[2], argv[3]);
  } else if(type == TankParams::token()) {
    doMerge<TankParams>(argv[1], argv[2], argv[3]);
  } else if(type == GloryParams::token()) {
    doMerge<GloryParams>(argv[1], argv[2], argv[3]);
  } else if(type == UpgradeParams::token()) {
    doMerge<UpgradeParams>(argv[1], argv[2], argv[3]);
  } else if(type == FactionParams::token()) {
    doMerge<FactionParams>(argv[1], argv[2], argv[3]);
  } else {
    CHECK(0);
  }
}
