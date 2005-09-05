#ifndef DNET_DVEC2
#define DNET_DVEC2

#include <string>
#include <vector>

#include "input.h"
#include "util.h"

using namespace std;

class Parameter {
public:
    
    string name;
    
    enum { BOOLEAN, BOUNDED_INTEGER };
    int type;
    
    bool hide_def;
    
    bool bool_val;
    bool bool_def;
    
    int bi_val;
    int bi_def;
    int bi_low;
    int bi_high;
    
    void update(const Button &l, const Button &r);
    void render(float x, float y, float h) const;
    
    string dumpTextRep() const;
    
};

Parameter paramBool(const string &name, bool begin, bool hideDefault);
Parameter paramBoundint(const string &name, int begin, int low, int high, bool hideDefault);

enum { ENTITY_TANKSTART, ENTITY_END };
static const char *ent_names[] = {"tank start location"};

class Entity {
public:
    
    int type;

    float x;
    float y;

    vector<Parameter> params;

    void initParams();  // inits params to the default for that type

};

struct VectorPoint {
    float x;
    float y;

    float curvlx;
    float curvly;
    float curvrx;
    float curvry;

    bool curvl;
    bool curvr;

    void mirror();
    void transform(const Transform2d &ctd);

    VectorPoint();
};

struct VectorPath {
};

struct Dvec2 {
public:
    vector<VectorPath> paths;
    vector<Entity> entities;
};

Dvec2 loadDvec2(const char *fname);

#endif
