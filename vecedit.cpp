
#include "vecedit.h"
#include "gfx.h"
#include "game.h" // currently just for Tank
#include "parse.h"

#include <vector>
#include <string>
#include <stack>
#include <fstream>

using namespace std;

enum { VECED_EXAMINE, VECED_PATH, VECED_REFLECT, VECED_MOVE, VECED_ENTITYTYPE, VECED_ENTITY, VECED_END };
const char *ed_names[] = { "Examine", "Path edit", "Reflect", "Move", "Entitytype", "Entity" };
enum { VECRF_SPIN, VECRF_SNOWFLAKE, VECRF_END };
const char *rf_names[] = {"spin", "snowflake"};
enum { ENTITY_TANKSTART, ENTITY_END };
const char *ent_names[] = {"tank start location"};

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

stack< int > modestack;

class Vecptn {
public:
    
    float x;
    float y;

    float curvlx;
    float curvly;
    float curvrx;
    float curvry;

    bool curvl;
    bool curvr;

    void mirror() {
        swap(curvlx, curvrx);
        swap(curvly, curvry);
        swap(curvl, curvr);
    }
    
    void transform(const Transform2d &ctd) {
        ctd.transform(&x, &y);
        ctd.transform(&curvlx, &curvly);
        ctd.transform(&curvrx, &curvry);
    }

    Vecptn() {
        x = y = 0;
        curvlx = curvly = curvrx = curvry = 16;
        curvl = curvr = false;
    }

};

class Path {
public:
    
    float centerx;
    float centery;

    int reflect;
    int dupes;

    int ang_denom;
    int ang_numer;

    vector<Vecptn> path;
    vector<Vecptn> vpath;

    int vpathCreate(int node); // used when nodes are created - the node is created before the given node id. returns the node's new ID (can be funky)
    void vpathModify(int node); // used when nodes are edited
    void vpathRemove(int node); // used when nodes are destroyed

    void moveCenterOrReflect(); // used when the center is moved or reflection is changed

    void setVRCurviness(int node, bool curv); // sets the R-curviness of the given virtual node

    Vecptn genNode(int i) const;
    vector<Vecptn> genFromPath() const;

    void fixCurve();
    void rebuildVpath();

    pair<int, bool> getCanonicalNode(int vnode) const;

    Path() {
        centerx = centery = 0;
        reflect = VECRF_SPIN;
        dupes = 1;
        ang_numer = 0;
        ang_denom = 1;
    }
    
};

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

void Parameter::update(const Button &l, const Button &r) {
    if(type == BOOLEAN) {
        if(l.repeat || r.repeat)
            bool_val = !bool_val;
    } else if(type == BOUNDED_INTEGER) {
        if(l.repeat)
            bi_val--;
        if(r.repeat)
            bi_val++;
        if(bi_val < bi_low)
            bi_val = bi_low;
        if(bi_val >= bi_high)
            bi_val = bi_high - 1;
        CHECK(bi_val >= bi_low && bi_val < bi_high);
    } else {
        CHECK(0);
    }
}

void Parameter::render(float x, float y, float h) const {
    string prefix = StringPrintf("%12s: ", name.c_str());
    if(type == BOOLEAN) {
        prefix += StringPrintf("%s", bool_val ? "true" : "false");
    } else if(type == BOUNDED_INTEGER) {
        prefix += StringPrintf("%d", bi_val);
    } else {
        CHECK(0);
    }
    drawText(prefix, h, x, y);
}

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

class Entity {
public:
    
    int type;

    float x;
    float y;

    vector<Parameter> params;

    void initParams();  // inits params to the default for that type

};

vector<Path> paths;
vector<Entity> entities;

int zoompow = 7;
int gridpow = 2;

float zoom = 64;
float grid = 4;

float cursor_x = 0;
float cursor_y = 0;

int Path::vpathCreate(int node) {
    CHECK(node >= 0 && node <= vpath.size());
    //dprintf("node is %d, vpath.size() is %d\n", vpath.size());
    pair<int, bool> can = getCanonicalNode(node);
    //dprintf("can is %d\n", can.first);
    if(can.second) can.first++;
    int phase = path.size() ? node / path.size() : 0;
    //dprintf("phase is %d\n", phase);
    // we're inserting a node to be the new can.first at dupe 0, but then we have to figure out what it ended up being at dupe #phase
    Vecptn tv;
    tv.x = cursor_x;
    tv.y = cursor_y;
    path.insert(path.begin() + can.first, tv);
    rebuildVpath();
    for(int i = 0; i < path.size(); i++) {
        pair<int, bool> ncan = getCanonicalNode(i + phase * path.size());
        if(ncan.first == can.first)
            return i + phase * path.size();
    }
    CHECK(0);
}

void Path::vpathModify(int node) {
    CHECK(node >= 0 && node < vpath.size());
    Vecptn orig = genNode(node);
    if(orig.curvl != vpath[node].curvl)
        setVRCurviness((node + vpath.size() - 1) % vpath.size(), vpath[node].curvl);
    if(orig.curvr != vpath[node].curvr)
        setVRCurviness(node, vpath[node].curvr);
    if(node >= path.size()) {
        pair<int, bool> canon = getCanonicalNode(node);
        Vecptn tnode = vpath[node];
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
}

void Path::setVRCurviness(int node, bool curv) {
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

void Path::vpathRemove(int node) {
    CHECK(node >= 0 && node < vpath.size());
    path.erase(path.begin() + getCanonicalNode(node).first);
    rebuildVpath();
}

Vecptn Path::genNode(int i) const {
    pair<int, bool> src = getCanonicalNode(i);
    Vecptn srcp = path[src.first];
    if(src.second)
        srcp.mirror();
    srcp.transform(rfg_behavior(reflect, i / path.size(), dupes, ang_numer, ang_denom));
    return srcp;
}

vector<Vecptn> Path::genFromPath() const {
    vector<Vecptn> nvpt;
    for(int i = 0; i < path.size() * rfg_repeats(reflect, dupes); i++)
        nvpt.push_back(genNode(i));
    return nvpt;
}

pair<int, bool> Path::getCanonicalNode(int vnode) const {
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

void Path::moveCenterOrReflect() {
    rebuildVpath();
}

void Path::fixCurve() {
    bool changed = false;
    vector<Vecptn> tvpath = genFromPath();
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

void Path::rebuildVpath() {
    fixCurve();
    vpath = genFromPath();
    CHECK(vpath.size() == path.size() * rfg_repeats(reflect, dupes));
    for(int i = 0; i < vpath.size(); i++)
        CHECK(vpath[i].curvr == vpath[(i + 1) % vpath.size()].curvl);
}

void Entity::initParams() {
    params.clear();
    if(type == ENTITY_TANKSTART) {
        params.push_back(paramBoundint("numerator", 0, 0, 100000, false));
        params.push_back(paramBoundint("denominator", 1, 1, 100000, false));
        params.push_back(paramBool("exist2", true, true));
        params.push_back(paramBool("exist3", true, true));
        params.push_back(paramBool("exist4", true, true));
        params.push_back(paramBool("exist5", true, true));
        params.push_back(paramBool("exist6", true, true));
        params.push_back(paramBool("exist7", true, true));
        params.push_back(paramBool("exist8", true, true));
        params.push_back(paramBool("exist9", true, true));
        params.push_back(paramBool("exist10", true, true));
        params.push_back(paramBool("exist11", true, true));
        params.push_back(paramBool("exist12", true, true));
    } else {
        CHECK(0);
    }
}

void savePath(int i, FILE *outfile) {
    CHECK(i >= 0 && i < paths.size());
    fprintf(outfile, "path {\n");
    fprintf(outfile, "  center=%f,%f\n", paths[i].centerx, paths[i].centery);
    fprintf(outfile, "  reflect=%s\n", rf_names[paths[i].reflect]);
    fprintf(outfile, "  dupes=%d\n", paths[i].dupes);
    fprintf(outfile, "  angle=%d/%d\n", paths[i].ang_numer, paths[i].ang_denom);
    for(int j = 0; j < paths[i].path.size(); j++) {
        string lhs;
        string rhs;
        if(paths[i].path[j].curvl)
            lhs = StringPrintf("%f,%f", paths[i].path[j].curvlx, paths[i].path[j].curvly);
        else
            lhs = "---";
        if(paths[i].path[j].curvr)
            rhs = StringPrintf("%f,%f", paths[i].path[j].curvrx, paths[i].path[j].curvry);
        else
            rhs = "---";
        fprintf(outfile, "  node= %s | %f,%f | %s\n", lhs.c_str(), paths[i].path[j].x, paths[i].path[j].y, rhs.c_str());
    }
    fprintf(outfile, "}\n");
    fprintf(outfile, "\n");
};

void saveEntity(int i, FILE *outfile) {
    CHECK(i >= 0 && i < entities.size());
    fprintf(outfile, "entity {\n");
    fprintf(outfile, "  type=%s\n", ent_names[entities[i].type]);
    fprintf(outfile, "  coord=%f,%f\n", entities[i].x, entities[i].y);
    for(int j = 0; j < entities[i].params.size(); j++)
        fprintf(outfile, "%s", entities[i].params[j].dumpTextRep().c_str());
    fprintf(outfile, "}\n");
    fprintf(outfile, "\n");
}

void saveDv2() {
    FILE *outfile;
    {
        char timestampbuf[ 128 ];
        time_t ctmt = time(NULL);
        strftime(timestampbuf, sizeof(timestampbuf), "%Y%m%d-%H%M%S.dv2", gmtime(&ctmt));
        dprintf("%s\n", timestampbuf);
        outfile = fopen(timestampbuf, "wb");
        if(!outfile) {
            dprintf("Outfile %s couldn't be opened! Didn't save!", timestampbuf);
            return;
        }
    }    
    for(int i = 0; i < paths.size(); i++)
        savePath(i, outfile);
    for(int i = 0; i < entities.size(); i++)
        saveEntity(i, outfile);
    fclose(outfile);
}

void loadDv2() {
    saveDv2();
    // Not yet implemented!
    CHECK(0);
}

int path_target = -1;
int path_curnode = -1;
int path_curhandle = -1;

int entity_target = -1;
int entity_position = -1;

int gui_vpos = 0;

float dsq(float xa, float ya, float xb, float yb) {
    return ( xa - xb ) * ( xa - xb ) + ( ya - yb ) * ( ya - yb );
}

float distSquared(float x, float y, int path, int node) {
    CHECK(path >= 0 && path < paths.size());
    CHECK(node >= -1 && node < (int)paths[path].vpath.size()); // damn size_t
    if(node != -1) {
        CHECK(node >= 0 && node < paths[path].vpath.size());
        return dsq(x, y, paths[path].centerx + paths[path].vpath[node].x, paths[path].centery + paths[path].vpath[node].y);
    } else {
        return dsq(x, y, paths[path].centerx, paths[path].centery);
    }
}

float handleDistSquared(float x, float y, int path, int node, bool side) {
    CHECK(path >= 0 && path < paths.size());
    CHECK(node >= 0 && node < paths[path].vpath.size());
    if(!side) {
        return dsq(x, y,
                        paths[path].centerx + paths[path].vpath[node].x + paths[path].vpath[node].curvlx,
                        paths[path].centery + paths[path].vpath[node].y + paths[path].vpath[node].curvly);
    } else {
        return dsq(x, y,
                        paths[path].centerx + paths[path].vpath[node].x + paths[path].vpath[node].curvrx,
                        paths[path].centery + paths[path].vpath[node].y + paths[path].vpath[node].curvry);
    }
}

pair<int, int> findTwoClosestNodes(float x, float y, int path) {
    CHECK(path >= 0 && path < paths.size());
    int closest = -1;
    int closest2 = -1;
    float best = 1000000000;
    float best2 = 1000000000;
    for(int i = 0; i < paths[path].vpath.size(); i++) {
        if(distSquared(x, y, path, i) < best) {
            closest2 = closest;
            best2 = best;
            closest = i;
            best = distSquared(x, y, path, i);
        } else if(distSquared(x, y, path, i) < best2) {
            closest2 = i;
            best2 = distSquared(x, y, path, i);
        }
    }
    CHECK(best <= best2);
    return make_pair(closest, closest2);
}

pair<int, bool> findClosestHandle(float x, float y, int path) {
    CHECK(path >= 0 && path < paths.size());
    int closestn = -1;
    bool closestr = false;
    float best = 1000000000;
    for(int i = 0; i < paths[path].vpath.size(); i++) {
        if(paths[path].vpath[i].curvl) {
            if(handleDistSquared(x, y, path, i, false) < best) {
                closestn = i;
                closestr = false;
                best = handleDistSquared(x, y, path, i, false);
            }
        }
        if(paths[path].vpath[i].curvr) {
            if(handleDistSquared(x, y, path, i, true) < best) {
                closestn = i;
                closestr = true;
                best = handleDistSquared(x, y, path, i, true);
            }
        }
    }
    return make_pair(closestn, closestr);
}

int findClosestPath(float x, float y) {
    int closest = -1;
    float best = 1000000000;
    for(int i = 0; i < paths.size(); i++) {
        if(distSquared(x, y, i, -1) < best) {
            closest = i;
            best = distSquared(x, y, i, -1);
        }
    }
    return closest;
}

int findClosestEntity(float x, float y) {
    int closest = -1;
    float best = 1000000000;
    for(int i = 0; i < entities.size(); i++) {
        if(dsq(x, y, entities[i].x, entities[i].y) < best) {
            best = dsq(x, y, entities[i].x, entities[i].y);
            closest = i;
        }
    }
    return closest;
}

void renderSinglePath(int path, float intense, int widgetlevel) {
    CHECK(path >= 0 && path < paths.size());
    
    const Path &p = paths[path];
    float linelen = zoom / 4;
    
    setColor(Color(1.0, 1.0, 1.0) * intense);
    
    // Render here!
    for(int i = 0; i < p.vpath.size(); i++) {
        int n = i + 1;
        n %= p.vpath.size();
        CHECK(p.vpath[i].curvr ==p.vpath[n].curvl);
        if(!p.vpath[i].curvr) {
            drawLine(p.centerx + p.vpath[i].x, p.centery + p.vpath[i].y, p.centerx + p.vpath[n].x, p.centery + p.vpath[n].y, 0.1);
        } else {
            drawCurve(Float4(
                p.centerx + p.vpath[i].x,                       p.centery + p.vpath[i].y,
                p.centerx + p.vpath[i].x + p.vpath[i].curvrx,   p.centery + p.vpath[i].y + p.vpath[i].curvry
            ), Float4(
                p.centerx + p.vpath[n].x + p.vpath[n].curvlx,   p.centery + p.vpath[n].y + p.vpath[n].curvly,
                p.centerx + p.vpath[n].x,                       p.centery + p.vpath[n].y
            ), 0.1);
        }
    }
    
    // reflection/core widget
    if(widgetlevel >= 1) {
        setColor(Color(0.5, 0.5, 0.5) * intense);
        drawBoxAround(p.centerx, p.centery, linelen / 10, 0.1);
        if(p.reflect == VECRF_SPIN) {
            setColor(Color(0.5, 0.5, 0.5) * intense);
            drawSpokes(p.centerx, p.centery, p.dupes, p.ang_numer, p.ang_denom, linelen, 0.1);
        } else if(p.reflect == VECRF_SNOWFLAKE) {
            setColor(Color(0.5, 0.5, 0.5) * intense);
            drawSpokes(p.centerx, p.centery, p.dupes, p.ang_numer, p.ang_denom, linelen, 0.1);
            setColor(Color(0.2, 0.2, 0.2) * intense);
            drawSpokes(p.centerx, p.centery, p.dupes, p.ang_numer * p.dupes * 2 + p.ang_denom, p.ang_denom * p.dupes * 2, linelen, 0.1);
        } else {
            CHECK(0);
        }
    }
    
    // handle widgets
    if(widgetlevel >= 2) {
        setColor(Color(0.3, 0.3, 0.3) * intense);
        for(int i = 0; i < p.vpath.size(); i++) {
            if(p.vpath[i].curvl)
                drawLine(p.centerx + p.vpath[i].x, p.centery + p.vpath[i].y, p.centerx + p.vpath[i].x + p.vpath[i].curvlx, p.centery + p.vpath[i].y + p.vpath[i].curvly, 0.1);
            if(p.vpath[i].curvr)
                drawLine(p.centerx + p.vpath[i].x, p.centery + p.vpath[i].y, p.centerx + p.vpath[i].x + p.vpath[i].curvrx, p.centery + p.vpath[i].y + p.vpath[i].curvry, 0.1);
        }
    }
    
    // closest widgets
    if(widgetlevel >= 3) {
        pair<int, int> close = findTwoClosestNodes(cursor_x, cursor_y, path);
        if(close.first != -1) {
            setColor(Color(0.0, 0.5, 0.0) * intense);
            drawBoxAround(p.centerx + p.vpath[close.first].x, p.centery + p.vpath[close.first].y, linelen / 20, 0.1);
        }
        if(close.second != -1) {
            setColor(Color(0.5, 0.0, 0.0) * intense);
            drawBoxAround(p.centerx + p.vpath[close.second].x, p.centery + p.vpath[close.second].y, linelen / 30, 0.1);
        }
        pair<int, bool> handle = findClosestHandle(cursor_x, cursor_y, path);
        if(handle.first != -1) {
            setColor(Color(0.0, 0.0, 0.5) * intense);
            if(!handle.second) {
                drawBoxAround(p.centerx + p.vpath[handle.first].x + p.vpath[handle.first].curvlx, p.centery + p.vpath[handle.first].y + p.vpath[handle.first].curvly, linelen / 30, 0.1);
            } else {
                drawBoxAround(p.centerx + p.vpath[handle.first].x + p.vpath[handle.first].curvrx, p.centery + p.vpath[handle.first].y + p.vpath[handle.first].curvry, linelen / 30, 0.1);
            }
        }
    }
    
}

void renderPaths() {
    for(int i = 0; i < paths.size(); i++) {
        float intense;
        int widgetlevel;
        if(path_target != -1 && i == path_target) {
            intense = 1;
            widgetlevel = 3;
        } else if(path_target != -1 && i != path_target) {
            intense = 0.5;
            widgetlevel = 0;
        } else {
            CHECK(path_target == -1);
            int close = findClosestPath(cursor_x, cursor_y);
            if(i == close) {
                intense = 1;
                widgetlevel = 2;
            } else {
                intense = 0.8;
                widgetlevel = 1;
            }
        }
        renderSinglePath(i, intense, widgetlevel);
    }
}

void renderSingleEntity(int p, int widgetlevel) {
    CHECK(p >= 0 && p < entities.size());
    const Entity &ent = entities[p];
    if(ent.type == ENTITY_TANKSTART) {
        CHECK(ent.params[0].name == "numerator");
        CHECK(ent.params[1].name == "denominator");
        setColor(1.0, 1.0, 1.0);
        drawLinePath(Tank().getTankVertices(ent.x, ent.y, (float)ent.params[0].bi_val / ent.params[1].bi_val * 2 * PI), 0.2, true);
        if(widgetlevel >= 1) {
            setZoom(0, 0, 100);
            for(int i = 0; i < ent.params.size(); i++) {
                if(entity_position == i) {
                    setColor(1.0, 1.0, 1.0);
                } else {
                    setColor(0.5, 0.5, 0.5);
                }
                ent.params[i].render(2, 12 + 3 * i, 2);
            }
            setZoom(-zoom*1.25, -zoom, zoom);
        }
    } else {
        CHECK(0);
    }
}

void renderEntities() {
    for(int i = 0; i < entities.size(); i++) {
        renderSingleEntity(i, i == entity_target);
    }
}

void guiText(const string &in) {
    setZoom(0, 0, 100);
    drawText(in, 2, 2, gui_vpos);
    gui_vpos += 3;
    setZoom(-zoom*1.25, -zoom, zoom);
}

bool vecEditTick(const Controller &keys) {
    
    // various consistency checks
    CHECK(sizeof(rf_names) / sizeof(*rf_names) == VECRF_END);
    CHECK(sizeof(ed_names) / sizeof(*ed_names) == VECED_END);
    
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
    
    if(modestack.size() == 0)   // get the whole shebang started
        modestack.push(VECED_EXAMINE);
    
    // these are guaranteed consistent behaviors
    if(keys.keys[0].repeat)
        zoompow--;
    if(keys.keys[1].repeat)
        zoompow++;
    if(keys.keys[2].repeat)
        gridpow--;
    if(keys.keys[3].repeat)
        gridpow++;
    
    zoom = pow(2., zoompow);
    grid = pow(2., gridpow);
    
    float *write_x = NULL;
    float *write_y = NULL;
    
    float offset_x = 0;
    float offset_y = 0;
    
    if(keys.keys[15].repeat && modestack.top() != VECED_EXAMINE) {
        if(modestack.top() == VECED_MOVE) {
            path_curhandle = -1;
            path_curnode = -1;
        }
        modestack.pop();
        if(modestack.top() == VECED_EXAMINE) {
            path_target = -1;
            entity_target = -1;
            entity_position = -1;
        }
    } else if(modestack.top() == VECED_EXAMINE) {
        if(keys.keys[4].repeat) {   // create path
            path_target = paths.size();
            paths.push_back(Path());
            paths[path_target].centerx = cursor_x;
            paths[path_target].centery = cursor_y;
            modestack.push(VECED_PATH);
            modestack.push(VECED_REFLECT);
        } else if(keys.keys[5].repeat) {    // create entity
            entity_target = entities.size();
            entities.push_back(Entity());
            entities[entity_target].x = cursor_x;
            entities[entity_target].y = cursor_y;
            entities[entity_target].type = 0;   // let's hope this is valid!
            entities[entity_target].initParams();
            modestack.push(VECED_ENTITY);
            modestack.push(VECED_ENTITYTYPE);
        } else if(keys.keys[8].repeat) {    // edit path
            int close = findClosestPath(cursor_x, cursor_y);
            if(close != -1) {
                path_target = close;
                modestack.push(VECED_PATH);
            }
        } else if(keys.keys[9].repeat) {    // edit entity
            int close = findClosestEntity(cursor_x, cursor_y);
            if(close != -1) {
                entity_target = close;
                modestack.push(VECED_ENTITY);
                cursor_x = entities[entity_target].x;
                cursor_y = entities[entity_target].y;
            }
        } else if(keys.keys[12].repeat) {   // delete path
            int close = findClosestPath(cursor_x, cursor_y);
            if(close != -1) {
                saveDv2();
                paths.erase(paths.begin() + close);
            }
        } else if(keys.keys[13].repeat) {   // delete entity
            int close = findClosestEntity(cursor_x, cursor_y);
            if(close != -1) {
                saveDv2();
                entities.erase(entities.begin() + close);
            }
        } else if(keys.keys[14].repeat) {   // load
            loadDv2();
        } else if(keys.keys[15].repeat) {   // save
            saveDv2();
        }
    } else if(modestack.top() == VECED_PATH) {
        CHECK(path_target >= 0 && path_target < paths.size());
        if(keys.keys[4].repeat) { // create node
            int createtarget = -1;
            {
                pair<int, int> close = findTwoClosestNodes(cursor_x, cursor_y, path_target);
                createtarget = close.first;
            }
            if(createtarget != -1) {
                int prev = createtarget + paths[path_target].vpath.size() - 1;
                int nxt = createtarget + 1;
                prev %= paths[path_target].vpath.size();
                nxt %= paths[path_target].vpath.size();
                float pd = distSquared(cursor_x, cursor_y, path_target, prev);
                float nd = distSquared(cursor_x, cursor_y, path_target, nxt);
                if(pd >= nd)
                    createtarget = nxt;
            } else {
                // fallback if there are zero nodes
                CHECK(paths[path_target].vpath.size() == 0);
                createtarget = 0;
            }
            path_curnode = paths[path_target].vpathCreate(createtarget);
            if(path_curnode != createtarget)
                dprintf("Corrected from %d to %d\n", createtarget, path_curnode);
            modestack.push(VECED_MOVE);
        } else if(keys.keys[8].repeat) { // edit node
            pair<int, int> close = findTwoClosestNodes(cursor_x, cursor_y, path_target);
            if(close.first != -1) {
                path_curnode = close.first;
                cursor_x = round(paths[path_target].centerx + paths[path_target].vpath[path_curnode].x);
                cursor_y = round(paths[path_target].centery + paths[path_target].vpath[path_curnode].y);
                modestack.push(VECED_MOVE);
            }
        } else if(keys.keys[12].repeat) { // delete node
            pair<int, int> close = findTwoClosestNodes(cursor_x, cursor_y, path_target);
            if(close.first != -1)
                paths[path_target].vpathRemove(close.first);
        } else if(keys.keys[5].repeat) { // node curviness
            pair<int, int> close = findTwoClosestNodes(cursor_x, cursor_y, path_target);
            if(close.first != -1) {
                paths[path_target].vpath[close.first].curvr = !paths[path_target].vpath[close.first].curvr;
                paths[path_target].vpathModify(close.first);
            }
        } else if(keys.keys[6].repeat) { // edit handle
            pair<int, bool> close = findClosestHandle(cursor_x, cursor_y, path_target);
            if(close.first != -1) {
                path_curnode = close.first;
                path_curhandle = close.second;
                if(!close.second) {
                    cursor_x = round(paths[path_target].centerx + paths[path_target].vpath[path_curnode].x + paths[path_target].vpath[path_curnode].curvlx);
                    cursor_y = round(paths[path_target].centery + paths[path_target].vpath[path_curnode].y + paths[path_target].vpath[path_curnode].curvly);
                } else {
                    cursor_x = round(paths[path_target].centerx + paths[path_target].vpath[path_curnode].x + paths[path_target].vpath[path_curnode].curvrx);
                    cursor_y = round(paths[path_target].centery + paths[path_target].vpath[path_curnode].y + paths[path_target].vpath[path_curnode].curvry);
                }
                modestack.push(VECED_MOVE);
            }
        } else if(keys.keys[9].repeat) { // center/reflect
            cursor_x = round(paths[path_target].centerx);
            cursor_y = round(paths[path_target].centery);
            modestack.push(VECED_REFLECT);
        }
    } else if(modestack.top() == VECED_REFLECT) {
        CHECK(path_target >= 0 && path_target < paths.size());
        if(keys.keys[4].repeat) // previous reflect
            paths[path_target].reflect--;
        if(keys.keys[5].repeat) // next reflect
            paths[path_target].reflect++;
        paths[path_target].reflect += VECRF_END;
        paths[path_target].reflect %= VECRF_END;
        if(keys.keys[8].repeat) // decrement dupes
            paths[path_target].dupes--;
        if(keys.keys[9].repeat) // increment dupes
            paths[path_target].dupes++;
        if(paths[path_target].dupes <= 0)
            paths[path_target].dupes = 1;
        if(keys.keys[6].repeat) // decrement numer
            paths[path_target].ang_numer--;
        if(keys.keys[7].repeat) // increment numer
            paths[path_target].ang_numer++;
        if(keys.keys[10].repeat) // decrement denom
            paths[path_target].ang_denom--;
        if(keys.keys[11].repeat) // increment denom
            paths[path_target].ang_denom++;
        if(paths[path_target].ang_denom <= 0)
            paths[path_target].ang_denom = 1;
        if(keys.keys[12].repeat || keys.keys[13].repeat) { // rotate/swap
            if(keys.keys[12].repeat) {
                paths[path_target].path.insert(paths[path_target].path.begin(), paths[path_target].path.back());
                paths[path_target].path.pop_back();
            } else if(keys.keys[13].repeat) {
                reverse(paths[path_target].path.begin(), paths[path_target].path.end());
            }
            paths[path_target].rebuildVpath();
        }
        write_x = &paths[path_target].centerx;
        write_y = &paths[path_target].centery;
    } else if(modestack.top() == VECED_MOVE) {
        // this is just write_x setup
    } else if(modestack.top() == VECED_ENTITYTYPE) {
        CHECK(entity_target >= 0 && entity_target < entities.size());
        if(keys.keys[4].repeat) // previous type
                entities[entity_target].type--;
        if(keys.keys[5].repeat) // next type
                entities[entity_target].type++;
        entities[entity_target].type += ENTITY_END;
        entities[entity_target].type %= ENTITY_END;
        entities[entity_target].initParams();
        write_x = &entities[entity_target].x;
        write_y = &entities[entity_target].y;
    } else if(modestack.top() == VECED_ENTITY) {
        CHECK(entity_target >= 0 && entity_target < entities.size());
        if(entity_position == -1)
            entity_position = 0;
        CHECK(entity_position >= 0 && entity_position < entities[entity_target].params.size());
        if(keys.keys[5].repeat)
            entity_position--;
        if(keys.keys[9].repeat)
            entity_position++;
        entity_position += entities[entity_target].params.size();
        entity_position %= entities[entity_target].params.size();
        entities[entity_target].params[entity_position].update(keys.keys[8], keys.keys[10]);
        write_x = &entities[entity_target].x;
        write_y = &entities[entity_target].y;
    } else {
        CHECK(0);
    }
    
    if(modestack.top() == VECED_MOVE) {
        CHECK(path_target >= 0 && path_target < paths.size());
        CHECK(path_curnode >= 0 && path_curnode < paths[path_target].vpath.size());
        if(path_curhandle == -1) {
            write_x = &paths[path_target].vpath[path_curnode].x;
            write_y = &paths[path_target].vpath[path_curnode].y;
            offset_x = paths[path_target].centerx;
            offset_y = paths[path_target].centery;
        } else {
            offset_x = paths[path_target].centerx + paths[path_target].vpath[path_curnode].x;
            offset_y = paths[path_target].centery + paths[path_target].vpath[path_curnode].y;
            if(path_curhandle == 0) {
                write_x = &paths[path_target].vpath[path_curnode].curvlx;
                write_y = &paths[path_target].vpath[path_curnode].curvly;
            } else if(path_curhandle == 1) {
                write_x = &paths[path_target].vpath[path_curnode].curvrx;
                write_y = &paths[path_target].vpath[path_curnode].curvry;
            } else {
                CHECK(0);
            }
        }
    }
    
    if(keys.l.repeat) cursor_x -= grid;
    if(keys.r.repeat) cursor_x += grid;
    if(keys.u.repeat) cursor_y -= grid;
    if(keys.d.repeat) cursor_y += grid;
    
    if(write_x) *write_x = cursor_x - offset_x;
    if(write_y) *write_y = cursor_y - offset_y;
        
    if(modestack.top() == VECED_REFLECT) {
        paths[path_target].moveCenterOrReflect();
        CHECK(path_target >= 0 && path_target < paths.size());
    }
    
    if(modestack.top() == VECED_MOVE) {
        CHECK(path_target >= 0 && path_target < paths.size());
        CHECK(path_curnode >= 0 && path_curnode < paths[path_target].vpath.size());
        paths[path_target].vpathModify(path_curnode);
    }
    
    /*if(paths.size()) {
        dprintf("---");
        for(int i = 0; i < paths[0].path.size(); i++)
            dprintf("%f, %f", paths[0].path[i].x, paths[0].path[i].y);
    }*/
    
    return false;
    
}

void vecEditRender() {
    setZoom(0, 0, 100);
    setColor(1.0f, 1.0f, 1.0f);
    gui_vpos = 2;
    guiText(StringPrintf("%s mode - 78 90 for zoom+grid (%f, %f)", ed_names[modestack.top()], zoom, grid));
    if(modestack.top() == VECED_EXAMINE) {
        guiText("u/j/m to create/edit/destroy paths         .// to load/save");
        guiText("i/k/, to create/edit/destroy entities      arrow keys to move");
    } else if(modestack.top() == VECED_PATH) {
        CHECK(path_target >= 0 && path_target < paths.size());
        guiText("u/j/m to create/edit/destroy nodes         / to exit");
        guiText("k to move center/reflect                   i/o for node curviness");
    } else if(modestack.top() == VECED_REFLECT) {
        CHECK(path_target >= 0 && path_target < paths.size());
        guiText("u/i j/k o/p l/; to change reflect modes");
        guiText(StringPrintf("current %s%d, origin %d/%d x 2PI", rf_names[paths[path_target].reflect], paths[path_target].dupes, paths[path_target].ang_numer, paths[path_target].ang_denom));
        guiText("arrow keys to move center, / to accept, m/, to rotate/reverse point order");
    } else if(modestack.top() == VECED_MOVE) {
        CHECK(path_target >= 0 && path_target < paths.size());
        CHECK(path_curnode >= 0 && path_curnode < paths[path_target].vpath.size());
        guiText("arrow keys to move, / to accept");
    } else if(modestack.top() == VECED_ENTITYTYPE) {
        CHECK(entity_target >= 0 && entity_target < entities.size());
        guiText("arrow keys to move, ui to change type, / to accept");
        guiText(StringPrintf("current type is %s", ent_names[entities[entity_target].type]));
    } else if(modestack.top() == VECED_ENTITY) {
        CHECK(entity_target >= 0 && entity_target < entities.size());
        guiText("arrow keys to move, ijkl to manipulate parameters, / to accept");
    } else {
        CHECK(0);
    }
    
    drawLine(cursor_x - grid, cursor_y, cursor_x - grid / 2, cursor_y, 0.1);
    drawLine(cursor_x + grid, cursor_y, cursor_x + grid / 2, cursor_y, 0.1);
    drawLine(cursor_x, cursor_y - grid, cursor_x, cursor_y - grid / 2, 0.1);
    drawLine(cursor_x, cursor_y + grid, cursor_x, cursor_y + grid / 2, 0.1);
    
    renderPaths();
    renderEntities();
    
}
