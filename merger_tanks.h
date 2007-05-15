#ifndef MERGER_TANKS
#define MERGER_TANKS

#include <string>
#include <vector>
#include <set>

#include "itemdb.h"
#include "parse.h"

using namespace std;

struct TankParams {
  struct Data {
    string health;
    string engine;
    string handling;
  
    string mass;
  };
  
  typedef IDBTank FinalType;
  
  static const bool twopass = false;
  
  static string token();
  
  static bool parseLine(const vector<string> &line, Data *data);
  static string getWantedName(const string &name, const set<string> &possiblenames);
  static void preprocess(kvData *kvd, const Data &data);
  static bool verify(const FinalType &item, const Data &data);
  
  static const map<string, FinalType> &finalTypeList();
};

#endif
