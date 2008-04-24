#ifndef MERGER_BOMBARDMENT
#define MERGER_BOMBARDMENT

#include <string>
#include <vector>
#include <set>

#include "itemdb.h"
#include "parse.h"
#include "merger_util.h"

using namespace std;

struct BombardParams {
  struct Data {
    string cost;
    
    string dpp;
    
    string lock;
    string unlock;
    
    string durability;
  };
  
  typedef IDBBombardment FinalType;
  
  static const bool twopass = false;
  
  typedef BaseNamer Namer;
  
  static string token();
  
  static bool isDemoable(const Data &toki);
  
  static bool parseLine(const vector<string> &line, Data *data);
  static const bool kvdirect = false;
  static string nameFromKvname(const string &name, const set<string> &possiblenames);
  static void preprocess(kvData *kvd, const Data &data);
  
  static vector<string> dependencies();
  static bool verify(const FinalType &item, const Data &data);
  
  static const map<string, FinalType> &finalTypeList();
};

#endif
