#ifndef MERGER_WEAPON
#define MERGER_WEAPON

#include <string>
#include <vector>
#include <set>

#include "itemdb.h"
#include "parse.h"

using namespace std;

struct WeaponParams {
  struct Data {
    string cost;
    string firerate;
    
    float dpp;
  };
  
  typedef IDBWeapon FinalType;
  
  static const bool twopass = true;
  
  static string token();
  
  static bool parseLine(const vector<string> &line, Data *data);
  static string getWantedName(const string &name, const set<string> &possiblenames);
  static void preprocess(kvData *kvd, const Data &data);
  static void testprocess(kvData *kvd, const Data &data);
  static float getMultiple(const FinalType &item, const Data &data);
  static void reprocess(kvData *kvd, const Data &data, float multiple);
  static bool verify(const FinalType &item, const Data &data);
  
  static const map<string, FinalType> &finalTypeList();
};

#endif
