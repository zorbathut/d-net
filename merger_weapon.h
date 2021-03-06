#ifndef MERGER_WEAPON
#define MERGER_WEAPON

#include "itemdb.h"
#include "parse.h"

#include <set>

using namespace std;

struct WeaponParams {
  struct Data {
    bool params;
    
    string item_cost;
    string item_firerate;
    string item_recommended;
    
    string params_threshold;
    string params_dethreshold;
    
    bool has_dpp;
    float dpp;
    string durability;
    
    bool demoable;
  };
  
  typedef IDBWeapon FinalType;
  
  static const bool twopass = true;
  
  class Namer {
    string prefix;
    int lastid;
    
  public:
    string getName(const vector<string> &line);
  
    Namer();
  };
  
  static string token();
  
  static bool isDemoable(const Data &toki);
  
  static bool parseLine(const vector<string> &line, Data *data);
  static const bool kvdirect = false;
  static string nameFromKvname(const string &name, const set<string> &possiblenames);
  static void preprocess(kvData *kvd, const Data &data);
  static void testprocess(kvData *kvd);
  static float getMultiple(const FinalType &item, const Data &data);
  static string getMultipleAltName(const string &name);
  static void reprocess(kvData *kvd, float multiple);
  
  static vector<string> dependencies();
  static bool verify(const FinalType &item, const Data &data);
  
  static const map<string, FinalType> &finalTypeList();
};

#endif
