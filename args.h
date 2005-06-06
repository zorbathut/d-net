#ifndef DNET_ARGS
#define DNET_ARGS

#include <map>
#include <string>

using namespace std;

#define DEFINE_string(id, def, descr) \
    string FLAGS_##id;\
    ARGS_LinkageObject id##_linkage(#id, &FLAGS_##id, def, descr);

#define DEFINE_bool(id, def, descr) \
    bool FLAGS_##id;\
    ARGS_LinkageObject id##_linkage(#id, &FLAGS_##id, def, descr);
    
class ARGS_LinkageObject {
public:
    ARGS_LinkageObject(const string &id, string *writeto, const string &def, const string &descr);
    ARGS_LinkageObject(const string &id, bool *writeto, bool def, const string &descr);
};

class LinkageData {
public:
    enum { LINKAGE_BOOL, LINKAGE_STRING };
    int type;
    
    string str_def;
    bool bool_def;
    
    string *str_link;
    bool *bool_link;
    
    LinkageData();
};

map< string, LinkageData > &getLinkageSingleton();

void initFlags(int argc, char *argv[]);

#endif
