#ifndef DNET_DUMPER_REGISTRY
#define DNET_DUMPER_REGISTRY

#include "args.h"

#include <string>
#include <map>

using namespace std;

#define REGISTER_VARIABLE(id, type) \
  REGISTER_LinkageObject id##_registerlinkage(#id, &FLAGS_##id, &FLAGS_##id##_OVERRIDDEN);

#define REGISTER_string(id) REGISTER_VARIABLE(id, string)
#define REGISTER_int(id) REGISTER_VARIABLE(id, int)
#define REGISTER_bool(id) REGISTER_VARIABLE(id, bool)
#define REGISTER_float(id) REGISTER_VARIABLE(id, float)

class REGISTER_LinkageObject : boost::noncopyable {
public:
  REGISTER_LinkageObject(const string &id, string *item, FlagSource *overridden);
  REGISTER_LinkageObject(const string &id, int *item, FlagSource *overridden);
  REGISTER_LinkageObject(const string &id, bool *item, FlagSource *overridden);
  REGISTER_LinkageObject(const string &id, float *item, FlagSource *overridden);
};

class RegistryData {
public:
  enum { REGISTRY_BOOL, REGISTRY_STRING, REGISTRY_INT, REGISTRY_FLOAT, REGISTRY_LAST };
  int type;
  
  FlagSource *source;
  
  string *str_link;
  int *int_link;
  bool *bool_link;
  float *float_link;
  
  RegistryData() { type = REGISTRY_LAST; }
};

map<string, RegistryData> &getRegistrationSingleton();

#endif
