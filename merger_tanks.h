#ifndef MERGER_TANKS
#define MERGER_TANKS

#include <string>
#include <vector>
#include <set>

#include "itemdb.h"
#include "parse.h"
#include "merger_util.h"

using namespace std;

struct TankParams {
  struct Data {
    string cost;
    
    string health;
    string engine;
    string handling;
  
    string mass;
  };
  
  typedef IDBTank FinalType;
  
  static const bool twopass = false;
  
  typedef BaseNamer Namer;
  
  static string token();
  
  static bool parseLine(const vector<string> &line, Data *data);
  static const bool kvdirect = false;
  static string nameFromKvname(const string &name, const set<string> &possiblenames);
  static void preprocess(kvData *kvd, const Data &data);
  static bool verify(const FinalType &item, const Data &data);
  
  static const map<string, FinalType> &finalTypeList();
};

#endif
