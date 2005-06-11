
#include "args.h"
#include "util.h"

ARGS_LinkageObject::ARGS_LinkageObject(const string &id, string *writeto, const string &def, const string &descr) {
    map< string, LinkageData > &links = getLinkageSingleton();
    LinkageData ld;
    ld.type = LinkageData::LINKAGE_STRING;
    ld.str_def = def;
    ld.str_link = writeto;
    links[id] = ld;
}
ARGS_LinkageObject::ARGS_LinkageObject(const string &id, bool *writeto, bool def, const string &descr) {
    map< string, LinkageData > &links = getLinkageSingleton();
    LinkageData ld;
    ld.type = LinkageData::LINKAGE_BOOL;
    ld.bool_def = def;
    ld.bool_link = writeto;
    links[id] = ld;
}

LinkageData::LinkageData() {
    type = -1;
    str_link = NULL;
    bool_link = NULL;
}

map< string, LinkageData > &getLinkageSingleton() {
    static map< string, LinkageData > singy;
    return singy;
}

void initFlags(int argc, char *argv[]) {
    map< string, LinkageData > &links = getLinkageSingleton();
    for(map<string, LinkageData>::iterator itr = links.begin(); itr != links.end(); itr++) {
        if(itr->second.type == LinkageData::LINKAGE_BOOL) {
            *itr->second.bool_link = itr->second.bool_def;
        } else if(itr->second.type == LinkageData::LINKAGE_STRING) {
            *itr->second.str_link = itr->second.str_def;
        } else {
            assert(0);
        }
    }
    for(int i = 1; i < argc; i++) {
        assert(argv[i][0] == '-' && argv[i][1] == '-');
        char *arg = argv[i] + 2;
        bool isBoolNo = false;
        if(strncmp(arg, "no", 2)) {
            isBoolNo = true;
            arg += 2;
        }
        char *eq = strchr(arg, '=');
        string realarg;
        if(eq) {
            realarg = string(arg, eq);
            eq++;
        } else {
            realarg = arg;
        }
        if(!links.count(realarg)) {
            dprintf("Invalid commandline argument %s\n", arg);
            assert(0);
            continue;
        }
        
        LinkageData &ld = links[realarg];
        if(ld.type == LinkageData::LINKAGE_BOOL) {
            assert(!eq || string(eq) == "true" || string(eq) == "false");
            assert(!eq || !isBoolNo);
            if(isBoolNo || (eq && string(eq) == "false")) {
                *ld.bool_link = false;
            } else {
                *ld.bool_link = true;
            }
        } else if(ld.type == LinkageData::LINKAGE_STRING) {
            assert(!isBoolNo && eq);
            *ld.str_link = eq;
        } else {
            assert(0);
        }
    }
}
