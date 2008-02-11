#ifndef DNET_ARGS
#define DNET_ARGS

#include <map>
#include <string>

#include <boost/noncopyable.hpp>

using namespace std;

enum FlagSource { FS_DEFAULT, FS_FILE, FS_CLI };

#define DECLARE_VARIABLE(id, type) \
  extern type FLAGS_##id; \
  extern FlagSource FLAGS_##id##_OVERRIDDEN;

#define DEFINE_VARIABLE(id, type, def, descr) \
  type FLAGS_##id;\
  FlagSource FLAGS_##id##_OVERRIDDEN;\
  ARGS_LinkageObject id##_linkage(#id, &FLAGS_##id, &FLAGS_##id##_OVERRIDDEN, def, descr); \
  char ***FLAGS_no##id __attribute__((unused)); // this only exists so we can't accidentally make two flags that only differ in "no"ness

#define DECLARE_string(id) DECLARE_VARIABLE(id, string)
#define DECLARE_int(id) DECLARE_VARIABLE(id, int)
#define DECLARE_bool(id) DECLARE_VARIABLE(id, bool)
#define DECLARE_float(id) DECLARE_VARIABLE(id, float)

#define DEFINE_string(id, def, descr) DEFINE_VARIABLE(id, string, def, descr)
#define DEFINE_int(id, def, descr) DEFINE_VARIABLE(id, int, def, descr)
#define DEFINE_bool(id, def, descr) DEFINE_VARIABLE(id, bool, def, descr)
#define DEFINE_float(id, def, descr) DEFINE_VARIABLE(id, float, def, descr)

class ARGS_LinkageObject : boost::noncopyable {
public:
  ARGS_LinkageObject(const string &id, string *writeto, FlagSource *source, const string &def, const string &descr);
  ARGS_LinkageObject(const string &id, int *writeto, FlagSource *source, int def, const string &descr);
  ARGS_LinkageObject(const string &id, bool *writeto, FlagSource *source, bool def, const string &descr);
  ARGS_LinkageObject(const string &id, float *writeto, FlagSource *source, float def, const string &descr);
};

map<string, string> getFlagDescriptions();

void initFlags(int argc, char *argv[], int ignoreargs, const string &settings = "");

#endif
