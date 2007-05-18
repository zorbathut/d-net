
#include "os.h"
#include "args.h"
#include "debug.h"
#include "merger_weapon.h"
#include "merger_bombardment.h"
#include "merger_tanks.h"
#include "merger_glory.h"
#include "merger_util.h"

#include <fstream>
#include <string>
#include <map>

using namespace std;

template<typename Model, bool twopass> struct PAW;

template<typename Model> struct PAW<Model, false> {
  static void f(const map<string, typename Model::Data> &tdd, const vector<kvData> &preproc, const string &merged) {
    ofstream ofs(merged.c_str());
    for(int i = 0; i < preproc.size(); i++) {
      checkForExtraMerges(preproc[i]);
      ofs << stringFromKvData(preproc[i]);
    }
  }
};

template<typename Model> struct PAW<Model, true> {
  static void f(const map<string, typename Model::Data> &tdd, const vector<kvData> &preproc, const string &merged) {
    set<string> names;
    for(typename map<string, typename Model::Data>::const_iterator itr = tdd.begin(); itr != tdd.end(); itr++)
      names.insert(itr->first);
    
    {
      ofstream ofs(merged.c_str());
      for(int i = 0; i < preproc.size(); i++) {
        string name = Model::getWantedName(preproc[i].read("name"), names);
        kvData kvd = preproc[i];
        if(name.size()) {
          CHECK(tdd.count(name));
          Model::testprocess(&kvd, tdd.find(name)->second);
        }
        checkForExtraMerges(kvd);
        ofs << stringFromKvData(kvd);
      }
    }
    
    addItemFile("data/base/common.dwh");
    addItemFile("data/base/hierarchy.dwh");
    addItemFile(merged);
    
    map<string, float> multipliers;
    for(typename map<string, typename Model::FinalType>::const_iterator itr = Model::finalTypeList().begin(); itr != Model::finalTypeList().end(); itr++) {
      string name = Model::getWantedName(itr->first, names);
      if(!name.size())
          continue;
      CHECK(tdd.count(name));
      CHECK(!multipliers.count(name));
      multipliers[suffix(itr->first)] = Model::getMultiple(itr->second, tdd.find(name)->second);
    }
    
    clearItemdb();
    
    {
      ofstream ofs(merged.c_str());
      for(int i = 0; i < preproc.size(); i++) {
        string name = Model::getWantedName(preproc[i].read("name"), names);
        kvData kvd = preproc[i];
        if(name.size()) {
          CHECK(tdd.count(name));
          CHECK(multipliers.count(name));
          Model::reprocess(&kvd, tdd.find(name)->second, multipliers[name]);
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

template<typename Model> void doMerge(const string &csv, const string &unmerged, const string &merged) {
  map<string, typename Model::Data> tdd;
  set<string> names;
  {
    ifstream ifs(csv.c_str());
    
    string lin;
    while(getline(ifs, lin)) {
      vector<string> dt = parseCsv(lin);
      
      if(dt[0] == Model::token() || dt[0] == "")
        continue;
      
      CHECK(!tdd.count(dt[0]));
      
      typename Model::Data dat;
      if(!Model::parseLine(dt, &dat))
        continue;
      
      tdd[dt[0]] = dat;
      names.insert(dt[0]);
    }
  }
  
  dprintf("Got %d items\n", tdd.size());
  
  vector<kvData> preproc;
  {
    set<string> done;
    ifstream ifs(unmerged.c_str());
    kvData kvd;
    while(getkvData(ifs, &kvd)) {
      string name = Model::getWantedName(kvd.read("name"), names);
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
  
  addItemFile("data/base/common.dwh");
  addItemFile("data/base/hierarchy.dwh");
  addItemFile(merged);
  
  for(typename map<string, typename Model::FinalType>::const_iterator itr = Model::finalTypeList().begin(); itr != Model::finalTypeList().end(); itr++) {
    CHECK(tdd.count(suffix(itr->first)));
    Model::verify(itr->second, tdd[suffix(itr->first)]);
    tdd.erase(suffix(itr->first));
  }
  CHECK(!tdd.size());
}  

int main(int argc, char *argv[]) {
  set_exename("merger.exe");
  initFlags(argc, argv, 3);
  
  CHECK(argc == 4);
  
  string type;
  {
    ifstream ifs(argv[1]);
    string line;
    CHECK(getline(ifs, line));
    type = parseCsv(line)[0];
  }
  dprintf("Got type %s\n", type.c_str());
  
  if(type == WeaponParams::token()) {
    doMerger<WeaponParams>(argv[1], argv[2], argv[3]);
  } else if(type == BombardParams::token()) {
    doMerge<BombardParams>(argv[1], argv[2], argv[3]);
  } else if(type == TankParams::token()) {
    doMerge<TankParams>(argv[1], argv[2], argv[3]);
  } else if(type == GloryParams::token()) {
    doMerge<GloryParams>(argv[1], argv[2], argv[3]);
  } else {
    CHECK(0);
  }
}
