#ifndef MERGER_TANKS
#define MERGER_TANKS

#include "itemdb.h"
#include "merger_util.h"
using namespace std;



struct TankParams {
  struct Data {
    string cost;
    
    string health;
    string engine;
    string handling;
  
    string mass;
    
    bool demoable;
  };
  
  typedef IDBTank FinalType;
  
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
