
#include "dumper_registry.h"

#include "util.h"

using namespace std;

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

map<string, RegistryData> &getRegistrationSingleton() {
  static map<string, RegistryData> singy;
  return singy;
}

REGISTER_LinkageObject::REGISTER_LinkageObject(const string &id, string *writeto, FlagSource *source) {
  map<string, RegistryData> &links = getRegistrationSingleton();
  RegistryData ld;
  ld.source = source;
  ld.type = RegistryData::REGISTRY_STRING;
  ld.str_link = writeto;
  links[id] = ld;
}
REGISTER_LinkageObject::REGISTER_LinkageObject(const string &id, int *writeto, FlagSource *source) {
  map<string, RegistryData> &links = getRegistrationSingleton();
  RegistryData ld;
  ld.source = source;
  ld.type = RegistryData::REGISTRY_INT;
  ld.int_link = writeto;
  links[id] = ld;
}
REGISTER_LinkageObject::REGISTER_LinkageObject(const string &id, bool *writeto, FlagSource *source) {
  map<string, RegistryData> &links = getRegistrationSingleton();
  RegistryData ld;
  ld.source = source;
  ld.type = RegistryData::REGISTRY_BOOL;
  ld.bool_link = writeto;
  links[id] = ld;
}
REGISTER_LinkageObject::REGISTER_LinkageObject(const string &id, float *writeto, FlagSource *source) {
  map<string, RegistryData> &links = getRegistrationSingleton();
  RegistryData ld;
  ld.source = source;
  ld.type = RegistryData::REGISTRY_FLOAT;
  ld.float_link = writeto;
  links[id] = ld;
}

map<string, string> getRegistryData() {
  map<string, string> dat;
  for(map<string, RegistryData>::iterator itr = getRegistrationSingleton().begin(); itr != getRegistrationSingleton().end(); itr++) {
    CHECK(!dat.count(itr->first));
    
    if(itr->second.type == RegistryData::REGISTRY_BOOL) {
      if(*itr->second.bool_link)
        dat[itr->first] = "true";
      else
        dat[itr->first] = "false";
    } else if(itr->second.type == RegistryData::REGISTRY_INT) {
      dat[itr->first] = StringPrintf("%d", *itr->second.int_link);
    } else if(itr->second.type == RegistryData::REGISTRY_FLOAT) {
      dat[itr->first] = string((const char *)itr->second.float_link, (const char *)(itr->second.float_link + 1));
    } else if(itr->second.type == RegistryData::REGISTRY_STRING) {
      dat[itr->first] = *itr->second.str_link;
    } else {
      CHECK(0);
    }
    
    CHECK(dat.count(itr->first));
  }
  
  return dat;
}

void setRegistryData(const map<string, string> &dat) {
  CHECK(dat.size() == getRegistrationSingleton().size());
  for(map<string, RegistryData>::iterator itr = getRegistrationSingleton().begin(); itr != getRegistrationSingleton().end(); itr++) {
    CHECK(dat.count(itr->first));
    CHECK(*itr->second.source != FS_CLI);
    *itr->second.source = FS_CLI; // owned
    
    // This is not ideal but I am doing it anyway.
    if(itr->second.type == RegistryData::REGISTRY_BOOL) {
      if(dat.find(itr->first)->second == "true") {
        *itr->second.bool_link = true;
      } else if(dat.find(itr->first)->second == "false") {
        *itr->second.bool_link = false;
      } else {
        CHECK(0);
      }
    } else if(itr->second.type == RegistryData::REGISTRY_INT) {
      *itr->second.int_link = atoi(dat.find(itr->first)->second.c_str());
    } else if(itr->second.type == RegistryData::REGISTRY_FLOAT) {
      CHECK(dat.find(itr->first)->second.size() == 4);
      *itr->second.float_link = *(float*)dat.find(itr->first)->second.c_str();
    } else if(itr->second.type == RegistryData::REGISTRY_STRING) {
      *itr->second.str_link = dat.find(itr->first)->second;
    } else {
      CHECK(0);
    }
  }
}
