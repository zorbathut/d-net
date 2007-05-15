#ifndef MERGER_BOMBARDMENT
#define MERGER_BOMBARDMENT

#include <string>
#include <vector>
#include <set>

#include "itemdb.h"
#include "parse.h"

using namespace std;

struct BombardParams {
  struct Data {
    string cost;
    
    string dpp;
    
    string lock;
    string unlock;
  };
  
  typedef IDBBombardment FinalType;
  
  static const bool twopass = false;
  
  static string token();
  
  static bool parseLine(const vector<string> &line, Data *data);
  static string getWantedName(const string &name, const set<string> &possiblenames);
  static void preprocess(kvData *kvd, const Data &data);
  static bool verify(const FinalType &item, const Data &data);
  
  static const map<string, FinalType> &finalTypeList();
};

#endif
