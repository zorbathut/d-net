#ifndef DNET_DVEC2
#define DNET_DVEC2

#include "float.h"

#include <string>
#include <map>

using namespace std;

// these are only needed if you're planning to edit the dvec2
class Transform2d;

enum { ENTITY_TANKSTART, ENTITY_END };
static const char *const ent_names[] = {"tank start location"};

struct Entity {
  int type;

  Float2 pos;

  int tank_ang_numer;
  int tank_ang_denom;
};

struct VectorPoint {
  Float2 pos;

  Float2 curvlp;
  Float2 curvrp;

  bool curvl;
  bool curvr;
  
  bool flat;

  void mirror();
  void transform(const Transform2d &ctd);

  VectorPoint();
};

enum { VECRF_SPIN, VECRF_SNOWFLAKE, VECRF_END };
static const char *const rf_names[] = {"spin", "snowflake"};

struct VectorPath {
  /*****
    * Stuff you should pay attention to no matter what you're doing
    */

  Float2 center;

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
  vector<VectorPath> paths;
  vector<Entity> entities;

  map<string, string> globals;

  Dvec2();

  Float4 boundingBox() const;
};

Dvec2 loadDvec2(const string &fname);

bool operator==(const Dvec2 &lhs, const Dvec2 &rhs);

#endif
