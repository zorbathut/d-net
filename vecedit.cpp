
#include "vecedit.h"
#include "gfx.h"

#include <vector>
#include <string>
#include <stack>

using namespace std;

class Transform2d {
public:
    float m[3][3];

    void hflip() {
        for(int i = 0; i < 3; i++)
            m[0][i] *= -1;
    }
    void vflip() {
        for(int i = 0; i < 3; i++)
            m[1][i] *= -1;
    }
    void dflip() {
        for(int i = 0; i < 3; i++)
            swap(m[0][i], m[1][i]);
    }
    
    float mx(float x, float y) const {
        float ox = 0.0;
        ox += m[0][0] * x;
        ox += m[0][1] * y;
        ox += m[0][2];
        return ox;
    }
    
    float my(float x, float y) const {
        float oy = 0.0;        
        oy += m[1][0] * x;
        oy += m[1][1] * y;
        oy += m[1][2];
        return oy;
    }
    
    void transform(float *x, float *y) const {
        float px = *x;
        float py = *y;
        *x = mx(px, py);
        *y = my(px, py);
    }
    
    void display() const {
        for(int i = 0; i < 3; i++)
            dprintf("  %f %f %f\n", m[0][i], m[1][i], m[2][i]);
    }

    Transform2d() {
        for(int i = 0; i < 3; i++)
            for(int j = 0; j < 3; j++)
                m[i][j] = ( i == j );
    }
};

inline Transform2d t2d_identity() {
    return Transform2d();
}
inline Transform2d t2d_flip(bool h, bool v, bool d) {
    Transform2d o;
    if(h)
        o.hflip();
    if(v)
        o.vflip();
    if(d)
        o.dflip();
    return o;
}

/*
what the fuck?
inline Transform2d t2d_rotate(float rad) {
    Transform2d o;
    o.m[0][0] = sin(rad);
    o.m[0][1] = cos(rad);
    o.m[1][0] = -cos(rad);
    o.m[1][1] = sin(rad);
    return o;
}
*/

enum { VECED_EXAMINE, VECED_PATH, VECED_REFLECT, VECED_MOVE };
const char *ed_names[] = { "Examine", "Path edit", "Reflect", "Move" };
enum { VECRF_NONE, VECRF_HORIZONTAL, VECRF_VERTICAL, VECRF_VH, VECRF_180DEG, VECRF_SNOWFLAKE4, VECRF_END };
const char *rf_names[] = {"none", "horizontal", "vertical", "vertical horizontal", "180deg", "snowflake4"};
const int rf_repeats[] = { 1, 2, 2, 4, 2, 8 };
const bool rf_mirror[] = { false, true, true, true, false, true };

Transform2d rf_none[] = {t2d_identity()};
Transform2d rf_horizontal[] = {t2d_identity(), t2d_flip(0,1,0)};
Transform2d rf_vertical[] = {t2d_identity(), t2d_flip(1,0,0)};
Transform2d rf_vh[] = {t2d_identity(), t2d_flip(1,0,0), t2d_flip(1,1,0), t2d_flip(0,1,0)};
Transform2d rf_180[] = {t2d_identity(), t2d_flip(1,1,0)};
Transform2d rf_snowflake4[] = {
        t2d_flip(0,0,0),
        t2d_flip(0,0,1),
        t2d_flip(0,1,1),
        t2d_flip(1,0,0),
        t2d_flip(1,1,0),
        t2d_flip(1,1,1),
        t2d_flip(1,0,1),
        t2d_flip(0,1,0)};

Transform2d *rf_behavior[] = {rf_none, rf_horizontal, rf_vertical, rf_vh, rf_180, rf_snowflake4};

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

    vector<Vecptn> path;
    vector<Vecptn> vpath;

    void vpathCreate(int node); // used when nodes are created - the node is created before the given node id
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
        reflect = VECRF_NONE;
    }
    
};

vector<Path> paths;
int zoom = 256;
int grid = 16;

int cursor_x = 0;
int cursor_y = 0;

float offset_x;
float offset_y;

void Path::vpathCreate(int node) {
    CHECK(node >= 0 && node <= vpath.size());
    pair<int, bool> can = getCanonicalNode(node);
    if(can.second) can.first++;
    Vecptn tv;
    tv.x = cursor_x;
    tv.y = cursor_y;
    path.insert(path.begin() + can.first, tv);
    rebuildVpath();
}

void Path::vpathModify(int node) {
    CHECK(node >= 0 && node < vpath.size());
    Vecptn orig = genNode(node);
    if(orig.curvl != vpath[node].curvl)
        setVRCurviness((node + vpath.size() - 1) % vpath.size(), vpath[node].curvl);
    if(orig.curvr != vpath[node].curvr)
        setVRCurviness(node, vpath[node].curvr);
    if(node >= path.size()) {
        dprintf("Not modifying, this has to be written!");
    } else {
        path[node] = vpath[node];
    }
    rebuildVpath();
}

void Path::setVRCurviness(int node, bool curv) {
    pair<int, bool> canoa = getCanonicalNode(node);
    pair<int, bool> canob = getCanonicalNode((node + 1) % (path.size() * rf_repeats[reflect]));
    dprintf("svrc: %d, %d %d, %d %d\n", node, canoa.first, canoa.second, canob.first, canob.second);
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
    srcp.transform(rf_behavior[reflect][i / path.size()]);
    return srcp;
}

vector<Vecptn> Path::genFromPath() const {
    vector<Vecptn> nvpt;
    for(int i = 0; i < path.size() * rf_repeats[reflect]; i++)
        nvpt.push_back(genNode(i));
    return nvpt;
}

pair<int, bool> Path::getCanonicalNode(int vnode) const {
    CHECK(vnode >= 0 && vnode <= path.size() * rf_repeats[reflect]);
    if(vnode == path.size() * rf_repeats[reflect]) {
        if(rf_mirror[reflect]) {
            CHECK(rf_repeats[reflect] % 2 == 0);
            return make_pair(0, false);
        } else {
            return make_pair(path.size(), false);
        }
        CHECK(0);
    }
    CHECK(vnode < path.size() * rf_repeats[reflect]);
    int bank = vnode / path.size();
    int sub = vnode % path.size();
    CHECK(bank >= 0 && bank < rf_repeats[reflect]);
    if(bank % 2 == 1 && rf_mirror[reflect]) {
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
    }
}

void Path::rebuildVpath() {
    fixCurve();
    vpath = genFromPath();
    CHECK(vpath.size() == path.size() * rf_repeats[reflect]);
    for(int i = 0; i < vpath.size(); i++)
        CHECK(vpath[i].curvr == vpath[(i + 1) % vpath.size()].curvl);
}

void savePaths() {
}

int path_target = -1;
int path_curnode = -1;
int path_curhandle = -1;

int gui_vpos = 0;

int distSquared(int x, int y, int path, int node) {
    CHECK(path >= 0 && path < paths.size());
    CHECK(node >= -1 && node < (int)paths[path].vpath.size()); // damn size_t
    if(node != -1) {
        CHECK(node >= 0 && node < paths[path].vpath.size());
        return round(pow(x - paths[path].centerx - paths[path].vpath[node].x, 2) + pow(y - paths[path].centery - paths[path].vpath[node].y, 2));
    } else {
        return round(pow(x - paths[path].centerx, 2) + pow(y - paths[path].centery, 2));
    }
}

int handleDistSquared(int x, int y, int path, int node, bool side) {
    CHECK(path >= 0 && path < paths.size());
    CHECK(node >= 0 && node < paths[path].vpath.size());
    if(!side) {
        return round(pow(x - paths[path].centerx - paths[path].vpath[node].x - paths[path].vpath[node].curvlx, 2) +
                     pow(y - paths[path].centery - paths[path].vpath[node].y - paths[path].vpath[node].curvly, 2));
    } else {
        return round(pow(x - paths[path].centerx - paths[path].vpath[node].x - paths[path].vpath[node].curvrx, 2) +
                     pow(y - paths[path].centery - paths[path].vpath[node].y - paths[path].vpath[node].curvry, 2));
    }
}

pair<int, int> findTwoClosestNodes(int x, int y, int path) {
    CHECK(path >= 0 && path < paths.size());
    int closest = -1;
    int closest2 = -1;
    int best = 1000000000;
    int best2 = 1000000000;
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

pair<int, bool> findClosestHandle(int x, int y, int path) {
    CHECK(path >= 0 && path < paths.size());
    int closestn = -1;
    bool closestr = false;
    int best = 1000000000;
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

int findClosestPath(int x, int y) {
    int closest = -1;
    int best = 1000000000;
    for(int i = 0; i < paths.size(); i++) {
        if(distSquared(x, y, i, -1) < best) {
            closest = i;
            best = distSquared(x, y, i, -1);
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
        if(p.reflect == VECRF_NONE) {
        } else if(p.reflect == VECRF_HORIZONTAL) {
            drawLine(p.centerx - linelen, p.centery, p.centerx + linelen, p.centery, 0.1);
        } else if(p.reflect == VECRF_VERTICAL) {
            drawLine(p.centerx, p.centery - linelen, p.centerx, p.centery + linelen, 0.1);
        } else if(p.reflect == VECRF_VH) {
            drawLine(p.centerx - linelen, p.centery, p.centerx + linelen, p.centery, 0.1);
            drawLine(p.centerx, p.centery - linelen, p.centerx, p.centery + linelen, 0.1);
        } else if(p.reflect == VECRF_180DEG) {
            drawLine(p.centerx - linelen, p.centery - linelen, p.centerx + linelen, p.centery + linelen, 0.1);
        } else if(p.reflect == VECRF_SNOWFLAKE4) {
            drawLine(p.centerx - linelen, p.centery - linelen, p.centerx + linelen, p.centery + linelen, 0.1);
            drawLine(p.centerx - linelen, p.centery + linelen, p.centerx + linelen, p.centery - linelen, 0.1);
            drawLine(p.centerx - linelen, p.centery, p.centerx + linelen, p.centery, 0.1);
            drawLine(p.centerx, p.centery - linelen, p.centerx, p.centery + linelen, 0.1);
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

void guiText(const string &in) {
    setZoom(0, 0, 100);
    drawText(in, 2, 2, gui_vpos);
    gui_vpos += 3;
    setZoom(-zoom*1.25, -zoom, zoom);
}

bool vecEditTick(const Controller &keys) {
    
    // various consistency checks
    CHECK(sizeof(rf_names) / sizeof(*rf_names) == VECRF_END);
    CHECK(sizeof(rf_repeats) / sizeof(*rf_repeats) == VECRF_END);
    CHECK(sizeof(rf_mirror) / sizeof(*rf_mirror) == VECRF_END);
    CHECK(sizeof(rf_behavior) / sizeof(*rf_behavior) == VECRF_END);
    
    if(modestack.size() == 0)   // get the whole shebang started
        modestack.push(VECED_EXAMINE);
    
    // these are guaranteed consistent behaviors
    if(keys.keys[0].repeat) {
        zoom /= 2;
        zoom = max(zoom, 1);
    }
    if(keys.keys[1].repeat) {
        zoom *= 2;
        zoom = max(zoom, 1);
    }
    if(keys.keys[2].repeat) {
        grid /= 2;
        grid = max(grid, 1);
    }
    if(keys.keys[3].repeat) {
        grid *= 2;
        grid = max(grid, 1);
    }
    
    float *write_x = NULL;
    float *write_y = NULL;
    
    if(keys.keys[15].repeat && modestack.top() != VECED_EXAMINE) {
        if(modestack.top() == VECED_MOVE) {
            path_curhandle = -1;
            path_curnode = -1;
        }
        modestack.pop();
        if(modestack.top() == VECED_EXAMINE) {
            path_target = -1;
        }
    } else if(modestack.top() == VECED_EXAMINE) {
        if(keys.keys[4].repeat) {   // create path
            path_target = paths.size();
            paths.push_back(Path());
            paths[path_target].centerx = cursor_x;
            paths[path_target].centery = cursor_y;
            modestack.push(VECED_PATH);
            modestack.push(VECED_REFLECT);
        } else if(keys.keys[8].repeat) {    // edit path
            int close = findClosestPath(cursor_x, cursor_y);
            if(close != -1) {
                path_target = close;
                modestack.push(VECED_PATH);
            }
        } else if(keys.keys[12].repeat) {   // delete path
            int close = findClosestPath(cursor_x, cursor_y);
            if(close != -1) {
                savePaths();
                paths.erase(paths.begin() + close);
            }
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
                int pd = distSquared(cursor_x, cursor_y, path_target, prev);
                int nd = distSquared(cursor_x, cursor_y, path_target, nxt);
                if(pd >= nd)
                    createtarget = nxt;
            } else {
                // fallback if there are zero nodes
                CHECK(paths[path_target].vpath.size() == 0);
                createtarget = 0;
            }
            paths[path_target].vpathCreate(createtarget);
            path_curnode = createtarget;
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
        write_x = &paths[path_target].centerx;
        write_y = &paths[path_target].centery;
        offset_x = 0;
        offset_y = 0;
    } else if(modestack.top() == VECED_MOVE) {
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
    } else {
        CHECK(0);
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
    
    return false;
    
}

void vecEditRender() {
    setZoom(0, 0, 100);
    setColor(1.0f, 1.0f, 1.0f);
    gui_vpos = 2;
    guiText(StringPrintf("%s mode - 78 90 for zoom+grid (%d, %d)", ed_names[modestack.top()], zoom, grid));
    if(modestack.top() == VECED_EXAMINE) {
        guiText("u/j/m to create/edit/destroy paths         .// to load/save");
        guiText("i/k/, to create/edit/destroy entities      arrow keys to move");
    } else if(modestack.top() == VECED_PATH) {
        CHECK(path_target >= 0 && path_target < paths.size());
        guiText("u/j/m to create/edit/destroy nodes         / to exit");
        guiText("k to move center/reflect                   i/o for node curviness");
    } else if(modestack.top() == VECED_REFLECT) {
        CHECK(path_target >= 0 && path_target < paths.size());
        guiText(StringPrintf("u/i to change reflect mode (currently %s)", rf_names[paths[path_target].reflect]));
        guiText("arrow keys to move center, / to accept");
    } else if(modestack.top() == VECED_MOVE) {
        CHECK(path_target >= 0 && path_target < paths.size());
        CHECK(path_curnode >= 0 && path_curnode < paths[path_target].vpath.size());
        guiText("arrow keys to move, / to accept");
    } else {
        CHECK(0);
    }
    
    drawLine(cursor_x - grid, cursor_y, cursor_x - grid / 2, cursor_y, 0.1);
    drawLine(cursor_x + grid, cursor_y, cursor_x + grid / 2, cursor_y, 0.1);
    drawLine(cursor_x, cursor_y - grid, cursor_x, cursor_y - grid / 2, 0.1);
    drawLine(cursor_x, cursor_y + grid, cursor_x, cursor_y + grid / 2, 0.1);
    
    renderPaths();
    
}
