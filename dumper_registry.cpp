
#include "dumper_registry.h"

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

