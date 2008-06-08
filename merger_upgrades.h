#ifndef MERGER_UPGRADES
#define MERGER_UPGRADES

#include "itemdb.h"
#include "parse.h"

#include <set>

using namespace std;

struct UpgradeParams {
  struct Data {
    string costmult;
    bool demoable;
  };
  
  typedef IDBUpgrade FinalType;
  
  static const bool twopass = false;
  
  class Namer {
  public:
    string getName(const vector<string> &line);
  };
  
  static string token();
  
  static bool isDemoable(const Data &toki);
  
  static bool parseLine(const vector<string> &line, Data *data);
  static const bool kvdirect = true;
  static string nameFromKvd(const kvData &kvd, const set<string> &possiblenames);
  static void preprocess(kvData *kvd, const Data &data);
  
  static vector<string> dependencies();
  static bool verify(const FinalType &item, const Data &data);
  
  static const map<string, FinalType> &finalTypeList();
};

#endif
