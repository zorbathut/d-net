#ifndef DNET_DVEC2
#define DNET_DVEC2

#include <string>
#include <vector>

#include "input.h"
#include "util.h"
#include "float.h"

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
    void parseTextRep(const string &in);
    
};

Parameter paramBool(const string &name, bool begin, bool hideDefault);
Parameter paramBoundint(const string &name, int begin, int low, int high, bool hideDefault);

enum { ENTITY_TANKSTART, ENTITY_END };
static const char *const ent_names[] = {"tank start location"};

struct Entity {
public:
    
    int type;

    float x;
    float y;

    vector<Parameter> params;

    void initParams();  // inits params to the default for that type

    Parameter *getParameter(const string &name);
    const Parameter *getParameter(const string &name) const;

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

enum { VECRF_SPIN, VECRF_SNOWFLAKE, VECRF_END };
static const char *const rf_names[] = {"spin", "snowflake"};

struct VectorPath {
public:
    
    /*****
      * Stuff you should pay attention to no matter what you're doing
      */

    float centerx;
    float centery;

    vector<VectorPoint> vpath;

    // this is not perfect, it doesn't deal with curves well. ATM I don't care. Can be fixed later.
    Float4 boundingBox() const;

    VectorPath();

    /*****
      * Stuff you should only pay attention to if you're loading or saving
      */

    int reflect;
    int dupes;

    int ang_denom;
    int ang_numer;

    vector<VectorPoint> path;

    void rebuildVpath();
    
    /*****
      * Stuff you should only pay attention to if you're actually editing the path
      */

    int vpathCreate(int node); // used when nodes are created - the node is created before the given node id. returns the node's new ID (can be funky)
    void vpathModify(int node); // used when nodes are edited
    void vpathRemove(int node); // used when nodes are destroyed

    void moveCenterOrReflect(); // used when the center is moved or reflection is changed

    void setVRCurviness(int node, bool curv); // sets the R-curviness of the given virtual node

    /*****
      * Stuff you should probably not pay attention to
      */
      
private:
    
    VectorPoint genNode(int i) const;
    vector<VectorPoint> genFromPath() const;

    void fixCurve();

    pair<int, bool> getCanonicalNode(int vnode) const;
    
};

struct Dvec2 {
public:
    vector<VectorPath> paths;
    vector<Entity> entities;

    Float4 boundingBox() const;
};

Dvec2 loadDvec2(const string &fname);

#endif
