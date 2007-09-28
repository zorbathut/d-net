#ifndef DNET_ARGS
#define DNET_ARGS

#include <map>
#include <string>

using namespace std;

#define DECLARE_string(id) \
  extern string FLAGS_##id;

#define DEFINE_string(id, def, descr) \
  string FLAGS_##id;\
  ARGS_LinkageObject id##_linkage(#id, &FLAGS_##id, def, descr);

#define DECLARE_int(id) \
  extern int FLAGS_##id;

#define DEFINE_int(id, def, descr) \
  int FLAGS_##id;\
  ARGS_LinkageObject id##_linkage(#id, &FLAGS_##id, def, descr);

#define DECLARE_bool(id) \
  extern bool FLAGS_##id;

#define DEFINE_bool(id, def, descr) \
  bool FLAGS_##id;\
  ARGS_LinkageObject id##_linkage(#id, &FLAGS_##id, def, descr);

#define DECLARE_float(id) \
  extern float FLAGS_##id;

#define DEFINE_float(id, def, descr) \
  float FLAGS_##id;\
  ARGS_LinkageObject id##_linkage(#id, &FLAGS_##id, def, descr);

class ARGS_LinkageObject {
public:
  ARGS_LinkageObject(const string &id, string *writeto, const string &def, const string &descr);
  ARGS_LinkageObject(const string &id, int *writeto, int def, const string &descr);
  ARGS_LinkageObject(const string &id, bool *writeto, bool def, const string &descr);
  ARGS_LinkageObject(const string &id, float *writeto, float def, const string &descr);
};

class LinkageData {
public:
  enum { LINKAGE_BOOL, LINKAGE_STRING, LINKAGE_INT, LINKAGE_FLOAT, LINKAGE_LAST };
  int type;
  
  string descr;
  
  string str_def;
  int int_def;
  bool bool_def;
  float float_def;
  
  string *str_link;
  int *int_link;
  bool *bool_link;
  float *float_link;
  
  LinkageData();
};

map<string, string> getFlagDescriptions();

void initFlags(int argc, char *argv[], int ignoreargs, const string &settings = "");

#endif
