#ifndef MERGER_FACTIONS
#define MERGER_FACTIONS

#include <string>
#include <vector>
#include <set>

#include "itemdb.h"
#include "parse.h"
#include "merger_util.h"

using namespace std;

struct FactionParams {
  struct Data {
    map<string, string> modifiers;
  };
  
  typedef IDBFaction FinalType;
  
  static const bool twopass = false;
  
  class Namer {
  public:
    string getName(const vector<string> &line);
  };
  
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
