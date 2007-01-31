
#include "dvec2.h"

#include "parse.h"
#include "util.h"
#include "const.h"

#include <fstream>

using namespace std;

string Parameter::dumpTextRep() const {
  if(type == BOOLEAN) {
    if(hide_def && bool_val == bool_def)
      return "";
    return "  " + name + "=" + (bool_val ? "true" : "false") + "\n";
  } else if(type == BOUNDED_INTEGER) {
    if(hide_def && bi_val == bi_def)
      return "";
    return StringPrintf("  %s=%d\n", name.c_str(), bi_val);
  } else {
    CHECK(0);
  }
}

void Parameter::parseTextRep(const string &in) {
  if(type == BOOLEAN) {
    if(in == "false") {
      bool_val = false;
    } else if(in == "true") {
      bool_val = true;
    } else {
      CHECK(0);
    }
  } else if(type == BOUNDED_INTEGER) {
    bi_val = atoi(in.c_str());
  } else {
    CHECK(0);
  }
}

Parameter paramBool(const string &name, bool begin, bool hideDefault) {
  Parameter param;
  param.name = name;
  param.type = Parameter::BOOLEAN;
  param.hide_def = hideDefault;
  param.bool_val = begin;
  param.bool_def = begin;
  return param;
};

Parameter paramBoundint(const string &name, int begin, int low, int high, bool hideDefault) {
  Parameter param;
  param.name = name;
  param.type = Parameter::BOUNDED_INTEGER;
  param.hide_def = hideDefault;
  param.bi_val = begin;
  param.bi_def = begin;
  param.bi_low = low;
  param.bi_high = high;
  return param;
};

void Entity::initParams() {
  params.clear();
  if(type == ENTITY_TANKSTART) {
    params.push_back(paramBoundint("numerator", 0, 0, 100000, false));
    params.push_back(paramBoundint("denominator", 1, 1, 100000, false));
    for(int i = 2; i <= 36; i++)
      params.push_back(paramBool(StringPrintf("exist%03d", i), true, true));
  } else {
    CHECK(0);
  }
}

Parameter *Entity::getParameter(const string &name) {
  for(int i = 0; i < params.size(); i++)
    if(params[i].name == name)
      return &params[i];
  return NULL;
}
const Parameter *Entity::getParameter(const string &name) const {
  for(int i = 0; i < params.size(); i++)
    if(params[i].name == name)
      return &params[i];
  return NULL;
}


void VectorPoint::mirror() {
  swap(curvlp, curvrp);
  swap(curvl, curvr);
}

void VectorPoint::transform(const Transform2d &ctd) {
  ctd.transform(&pos);
  ctd.transform(&curvlp);
  ctd.transform(&curvrp);
}

VectorPoint::VectorPoint() {
  pos = Float2(0, 0);
  curvlp = Float2(0, 0);
  curvrp = Float2(0, 0);
  curvl = curvr = false;
}

int rfg_repeats(int type, int dupe) {
  CHECK(dupe > 0);
  if(type == VECRF_SPIN)
    return dupe;
  else if(type == VECRF_SNOWFLAKE)
    return dupe * 2;
  else
    CHECK(0);
}

bool rfg_mirror(int type) {
  if(type == VECRF_SPIN)
    return false;
  else if(type == VECRF_SNOWFLAKE)
    return true;
  else
    CHECK(0);
}

Transform2d rfg_behavior(int type, int segment, int dupe, int numer, int denom) {
  if(type == VECRF_SPIN) {
    return t2d_rotate((float)segment / dupe * 2 * PI);
  } else if(type == VECRF_SNOWFLAKE) {
    int phase = segment / 2;
    bool flipped = segment % 2;
    // Step 1: We're offset from the 0 position by numer / denom. So first we rotate in that direction.
    Transform2d base;
    base *= t2d_rotate((float)numer / denom * 2 * PI);
    // Step 2: If we're flipped, we flip.
    if(flipped)
      base *= t2d_flip(0, 1, 0);
    // Step 3: Return to the 0 position.
    base *= t2d_rotate(-(float)numer / denom * 2 * PI);
    // Step 4: Rotate appropriately.
    base *= t2d_rotate((float)phase / dupe * 2 * PI);
    return base;
  } else {
    CHECK(0);
  }
}

Float4 VectorPath::boundingBox() const {
  Float4 bbox = startFBoundBox();
  for(int i = 0; i < vpath.size(); i++)
    addToBoundBox(&bbox, center + vpath[i].pos);
  return bbox;
}

int VectorPath::vpathCreate(int node) {
  CHECK(node >= 0 && node <= vpath.size());
  //dprintf("node is %d, vpath.size() is %d\n", vpath.size());
  pair<int, bool> can = getCanonicalNode(node);
  //dprintf("can is %d\n", can.first);
  if(can.second) can.first++;
  int phase = path.size() ? node / path.size() : 0;
  //dprintf("phase is %d\n", phase);
  // we're inserting a node to be the new can.first at dupe 0, but then we have to figure out what it ended up being at dupe #phase
  VectorPoint tv;
  tv.pos = Float2(0, 0);
  tv.curvl = tv.curvr = vpath[node].curvl;
  path.insert(path.begin() + can.first, tv);
  fixCurve();
  rebuildVpath();
  for(int i = 0; i < path.size(); i++) {
    pair<int, bool> ncan = getCanonicalNode(i + phase * path.size());
    if(ncan.first == can.first)
      return i + phase * path.size();
  }
  CHECK(0);
}

void setDefaultCurvs(VectorPoint *vpl, VectorPoint *vpr) {
  if(vpl->curvrp == Float2(0, 0)) {
    dprintf("curvr\n");
    vpl->curvrp = (vpr->pos - vpl->pos) / 3;
  }
  if(vpr->curvlp == Float2(0, 0)) {
    dprintf("curvl\n");
    vpr->curvlp = (vpl->pos - vpr->pos) / 3;
  }
}

void VectorPath::vpathModify(int node) {
  CHECK(node >= 0 && node < vpath.size());
  VectorPoint orig = genNode(node);
  pair<int, int> defcurv = make_pair(-1, -1);
  if(orig.curvl != vpath[node].curvl) {
    if(vpath[node].curvl) {
      CHECK(defcurv.first == -1);
      defcurv = make_pair((node + vpath.size() - 1) % vpath.size(), node);
    }
    setVRCurviness((node + vpath.size() - 1) % vpath.size(), vpath[node].curvl);
  }
  if(orig.curvr != vpath[node].curvr) {
    if(vpath[node].curvr) {
      CHECK(defcurv.first == -1);
      defcurv = make_pair(node, (node + 1) % vpath.size());
    }
    setVRCurviness(node, vpath[node].curvr);
  }
  if(node >= path.size()) {
    pair<int, bool> canon = getCanonicalNode(node);
    VectorPoint tnode = vpath[node];
    if(canon.second)
      tnode.mirror();
    Transform2d trans = rfg_behavior(reflect, node / path.size(), dupes, ang_numer, ang_denom);
    trans.invert();
    tnode.transform(trans);
    path[canon.first] = tnode;
  } else {
    path[node] = vpath[node];
  }
  rebuildVpath();
  
  if(defcurv.first != -1) {
    // let's do some rather icky curve modification stuff
    setDefaultCurvs(&vpath[defcurv.first], &vpath[defcurv.second]);
    vpathModify(defcurv.first);
    setDefaultCurvs(&vpath[defcurv.first], &vpath[defcurv.second]);
    vpathModify(defcurv.second);
  }
}

void VectorPath::setVRCurviness(int node, bool curv) {
  pair<int, bool> canoa = getCanonicalNode(node);
  pair<int, bool> canob = getCanonicalNode((node + 1) % (path.size() * rfg_repeats(reflect, dupes)));
  //dprintf("svrc: %d, %d %d, %d %d\n", node, canoa.first, canoa.second, canob.first, canob.second);
  if(!canoa.second)
    path[canoa.first].curvr = curv;
  else
    path[canoa.first].curvl = curv;
  if(!canob.second)
    path[canob.first].curvl = curv;
  else
    path[canob.first].curvr = curv;
}

void VectorPath::vpathRemove(int node) {
  CHECK(node >= 0 && node < vpath.size());
  path.erase(path.begin() + getCanonicalNode(node).first);
  fixCurve();
  rebuildVpath();
}

VectorPoint VectorPath::genNode(int i) const {
  pair<int, bool> src = getCanonicalNode(i);
  VectorPoint srcp = path[src.first];
  if(src.second)
    srcp.mirror();
  srcp.transform(rfg_behavior(reflect, i / path.size(), dupes, ang_numer, ang_denom));
  return srcp;
}

vector<VectorPoint> VectorPath::genFromPath() const {
  vector<VectorPoint> nvpt;
  for(int i = 0; i < path.size() * rfg_repeats(reflect, dupes); i++)
    nvpt.push_back(genNode(i));
  return nvpt;
}

pair<int, bool> VectorPath::getCanonicalNode(int vnode) const {
  CHECK(vnode >= 0 && vnode <= path.size() * rfg_repeats(reflect, dupes));
  if(vnode == path.size() * rfg_repeats(reflect, dupes)) {
    if(rfg_mirror(reflect)) {
      CHECK(rfg_repeats(reflect, dupes) % 2 == 0);
      return make_pair(0, false);
    } else {
      return make_pair(path.size(), false);
    }
    CHECK(0);
  }
  CHECK(vnode < path.size() * rfg_repeats(reflect, dupes));
  int bank = vnode / path.size();
  int sub = vnode % path.size();
  CHECK(bank >= 0 && bank < rfg_repeats(reflect, dupes));
  if(bank % 2 == 1 && rfg_mirror(reflect)) {
    // this is a mirrored node
    return make_pair(path.size() - sub - 1, true);
  } else {
    // this is a normal node
    return make_pair(sub, false);
  }
}

void VectorPath::moveCenterOrReflect() {
  fixCurve();
  rebuildVpath();
}

void VectorPath::fixCurve() {
  bool changed = false;
  vector<VectorPoint> tvpath = genFromPath();
  for(int i = 0; i < tvpath.size(); i++) {
    int n = (i + 1) % tvpath.size();
    if(tvpath[i].curvr != tvpath[n].curvl) {
      dprintf("Splicing curves");
      setVRCurviness(i, true);
      changed = true;
    }
  }
  /*
  if(changed) {
    dprintf("Curves have been spliced");
    dprintf("Before:");
    for(int i = 0; i < tvpath.size(); i++)
      dprintf("%d %d", tvpath[i].curvl, tvpath[i].curvr);
    tvpath = genFromPath();
    dprintf("After:");
    for(int i = 0; i < tvpath.size(); i++)
      dprintf("%d %d", tvpath[i].curvl, tvpath[i].curvr);
    for(int i = 0; i < tvpath.size(); i++)
      CHECK(tvpath[i].curvr == tvpath[(i + 1) % tvpath.size()].curvl);
  }*/
}

void VectorPath::rebuildVpath() {
  vpath = genFromPath();
  CHECK(vpath.size() == path.size() * rfg_repeats(reflect, dupes));
  {
    bool curveerror = false;
    for(int i = 0; i < vpath.size(); i++)
      if(vpath[i].curvr != vpath[(i + 1) % vpath.size()].curvl)
        curveerror = true;
    if(curveerror) {
      dprintf("Curve consistency error! %d, %d\n", path.size(), vpath.size());
      for(int i = 0; i < vpath.size(); i++) {
        if(i % path.size() == 0)
          dprintf("--");
        dprintf("%d %d\n", vpath[i].curvl, vpath[i].curvr);
      }
      CHECK(0);
    }
  }
}

VectorPath::VectorPath() {
  center = Float2(0, 0);
  reflect = VECRF_SPIN;
  dupes = 1;
  ang_numer = 0;
  ang_denom = 1;
}

Float4 Dvec2::boundingBox() const {
  Float4 bb = startFBoundBox();
  for(int i = 0; i < paths.size(); i++)
    addToBoundBox(&bb, paths[i].boundingBox());
  for(int i = 0; i < entities.size(); i++)
    addToBoundBox(&bb, entities[i].pos);
  return bb;
}

Dvec2::Dvec2() {
  scale = 1.0;
}

Dvec2 loadDvec2(const string &fname) {
  dprintf("Loading %s\n", fname.c_str());
  Dvec2 rv;
  kvData dat;
  ifstream fil(fname.c_str());
  CHECK(fil);
  bool got_params = false;
  while(getkvData(fil, &dat)) {
    if(dat.category == "path") {
      VectorPath nvp;
      sscanf(dat.consume("center").c_str(), "%f,%f", &nvp.center.x, &nvp.center.y);
      
      nvp.reflect = find(rf_names, rf_names + VECRF_END, dat.consume("reflect")) - rf_names;
      CHECK(nvp.reflect >= 0 && nvp.reflect < VECRF_END);
      
      nvp.dupes = atoi(dat.consume("dupes").c_str());
      sscanf(dat.consume("angle").c_str(), "%d/%d", &nvp.ang_numer, &nvp.ang_denom);
      
      vector<string> nodes = tokenize(dat.consume("node"), "\n");
      for(int i = 0; i < nodes.size(); i++) {
        vector<string> tnode = tokenize(nodes[i], " |");
        CHECK(tnode.size() == 3);
        VectorPoint vpt;
        sscanf(tnode[1].c_str(), "%f,%f", &vpt.pos.x, &vpt.pos.y);
        if(tnode[0] == "---") {
          vpt.curvl = false;
        } else {
          vpt.curvl = true;
          sscanf(tnode[0].c_str(), "%f,%f", &vpt.curvlp.x, &vpt.curvlp.y);
        }
        if(tnode[2] == "---") {
          vpt.curvr = false;
        } else {
          vpt.curvr = true;
          sscanf(tnode[2].c_str(), "%f,%f", &vpt.curvrp.x, &vpt.curvrp.y);
        }
        nvp.path.push_back(vpt);
      }
      
      nvp.rebuildVpath();
      rv.paths.push_back(nvp);
    } else if(dat.category == "entity") {
      Entity ne;
      sscanf(dat.consume("coord").c_str(), "%f,%f", &ne.pos.x, &ne.pos.y);
      string typ = dat.consume("type");
      ne.type = -1;
      for(int i = 0; i < ENTITY_END; i++)
        if(typ == ent_names[i])
          ne.type = i;
      CHECK(ne.type != -1);
      ne.initParams();
      for(int i = 0; i < ne.params.size(); i++)
        if(dat.kv.count(ne.params[i].name))
          ne.params[i].parseTextRep(dat.consume(ne.params[i].name));
      rv.entities.push_back(ne);
    } else if(dat.category == "params") {
      CHECK(!got_params);
      got_params = true;
      rv.scale = atof(dat.consume("scale").c_str());
    } else {
      CHECK(0);
    }
    dat.shouldBeDone();
  }
  return rv;
}

/*
// this code should probably be salvaged sometime
  {
    static int firstrun = 1;
    if(firstrun) {
      firstrun = 0;
      // Test my matrix inversion! Wheeee
      for(int i = 0; i < VECRF_END; i++) {
        for(int j = 0; j < rfg_repeats(i, 1); j++) {
          Transform2d orig = rfg_behavior(i, j, 1, 0, 1);
          Transform2d inv = orig;
          inv.invert();
          //dprintf("-----");
          //orig.display();
          //dprintf("--");
          //inv.display();
          inv = orig * inv;
          //dprintf("--");
          //inv.display();
          for(int x = 0; x < 3; x++) {
            for(int y = 0; y < 3; y++) {
              CHECK(fabs(inv.m[x][y] - (x == y)) < 1e-9);
            }
          }
        }
      }
    }
  }
*/
