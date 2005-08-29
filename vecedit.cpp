
#include "vecedit.h"
#include "gfx.h"

#include <vector>
#include <string>
#include <stack>

using namespace std;

enum { VECED_EXAMINE, VECED_PATH, VECED_REFLECT, VECED_MOVE };
const char *ed_names[] = { "Examine", "Path edit", "Reflect", "Move" };
enum { VECRF_NONE, VECRF_HORIZONTAL, VECRF_VERTICAL, VECRF_VH, VECRF_180DEG, VECRF_SNOWFLAKE4, VECRF_END };
const char *rf_names[] = {"none", "horizontal", "vertical", "vertical horizontal", "180deg", "snowflake4"};
const int rf_repeats[] = { 1, 2, 2, 4, 2, 8 };
const bool rf_mirror[] = { false, true, true, true, false, true };

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
inline Transform2d t2d_rotate(float rad) {
    Transform2d o;
    o.m[0][0] = sin(rad);
    o.m[0][1] = cos(rad);
    o.m[1][0] = -cos(rad);
    o.m[1][1] = sin(rad);
    return o;
}

Transform2d rf_none[] = {t2d_identity()};
Transform2d rf_horizontal[] = {t2d_identity(), t2d_flip(1,0,0)};
Transform2d rf_vertical[] = {t2d_identity(), t2d_flip(0,1,0)};
Transform2d rf_vh[] = {t2d_identity(), t2d_flip(1,0,0), t2d_flip(1,1,0), t2d_flip(0,1,0)};
Transform2d rf_180[] = {t2d_identity(), t2d_rotate(PI)};
Transform2d rf_snowflake4[] = {
        t2d_flip(0,0,0),
        t2d_flip(0,0,1),
        t2d_flip(0,1,1),
        t2d_flip(1,0,0),
        t2d_flip(1,1,0),
        t2d_flip(1,1,1),
        t2d_flip(0,1,1),
        t2d_flip(0,1,0) };

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

    Vecptn() {
        x = y = 0;
        curvlx = curvly = curvrx = curvry = 0;
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
    vector<Vecptn> genFromPath() const;

    void checkConsistency() const;

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
    CHECK(reflect == VECRF_NONE);
    CHECK(node >= 0 && node <= path.size());
    Vecptn tv;
    tv.x = cursor_x;
    tv.y = cursor_y;
    path.insert(path.begin() + node, tv);
    vpath = genFromPath();
    checkConsistency();
}

void Path::vpathModify(int node) {
    CHECK(reflect == VECRF_NONE);
    CHECK(node >= 0 && node <= path.size());
    path = vpath;
    checkConsistency();
}

void Path::vpathRemove(int node) {
    CHECK(reflect == VECRF_NONE);
    CHECK(node >= 0 && node <= path.size());
    path.erase(path.begin() + node);
    vpath = genFromPath();
    checkConsistency();
}

void Path::moveCenterOrReflect() {
    vpath = genFromPath();
}

vector<Vecptn> Path::genFromPath() const {
    CHECK(reflect == VECRF_NONE);
    return path;
}

void Path::checkConsistency() const {
    for(int i = 0; i < vpath.size(); i++) {
        CHECK(vpath[i].curvl == false);
        CHECK(vpath[i].curvr == false);
    }
}

void savePaths() {
}

int path_target = -1;
int path_curnode = -1;

int gui_vpos = 0;

int distSquared(int x, int y, int path, int node) {
    CHECK(path >= 0 && path < paths.size());
    if(node != -1) {
        CHECK(node >= 0 && node < paths[path].vpath.size());
        return round(pow(x - paths[path].vpath[node].x - paths[path].centerx, 2) + pow(y - paths[path].vpath[node].y - paths[path].centery, 2));
    } else {
        return round(pow(x - paths[path].centerx, 2) + pow(y - paths[path].centery, 2));
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

void renderSinglePath(int path) {
    CHECK(path >= 0 && path < paths.size());
    
    const Path &p = paths[path];
    float linelen = zoom / 4;
    setColor(0.5, 0.5, 0.5);
    if(p.reflect == VECRF_NONE) {
        drawBoxAround(p.centerx, p.centery, linelen / 10, 0.1);
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
    
    setColor(1.0, 1.0, 1.0);
    
    // Render here!
    for(int i = 0; i < p.vpath.size(); i++) {
        int n = i + 1;
        n %= p.vpath.size();
        CHECK(p.vpath[i].curvr == false);
        drawLine(p.centerx + p.vpath[i].x, p.centery + p.vpath[i].y, p.centerx + p.vpath[n].x, p.centery + p.vpath[n].y, 0.1);
    }
    
}

void renderPaths() {
    for(int i = 0; i < paths.size(); i++) {
        renderSinglePath(i);
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
        modestack.pop();
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
                cursor_x = round(paths[path_target].vpath[path_curnode].x + paths[path_target].centerx);
                cursor_y = round(paths[path_target].vpath[path_curnode].y + paths[path_target].centery);
                modestack.push(VECED_MOVE);
            }
        } else if(keys.keys[12].repeat) { // delete node
            pair<int, int> close = findTwoClosestNodes(cursor_x, cursor_y, path_target);
            if(close.first != -1)
                paths[path_target].vpathRemove(close.first);
        } else if(keys.keys[9].repeat) { // center/reflect
            cursor_x = round(paths[path_target].centerx);
            cursor_y = round(paths[path_target].centery);
            modestack.push(VECED_REFLECT);
        } else if(keys.keys[15].repeat) {
            path_target = -1;
            path_curnode = -1;
            modestack.pop();
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
    } else if(modestack.top() ==  VECED_MOVE) {
        CHECK(path_target >= 0 && path_target < paths.size());
        CHECK(path_curnode >= 0 && path_curnode < paths[path_target].vpath.size());
        write_x = &paths[path_target].vpath[path_curnode].x;
        write_y = &paths[path_target].vpath[path_curnode].y;
        offset_x = -paths[path_target].centerx;
        offset_y = -paths[path_target].centery;
    } else {
        CHECK(0);
    }
    
    if(keys.l.repeat) cursor_x -= grid;
    if(keys.r.repeat) cursor_x += grid;
    if(keys.u.repeat) cursor_y -= grid;
    if(keys.d.repeat) cursor_y += grid;
    
    if(write_x) *write_x = cursor_x + offset_x;
    if(write_y) *write_y = cursor_y + offset_y;
        
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
        guiText("k to move center/reflect");
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
/*
enum { VECED_REFLECT, VECED_CREATE, VECED_EXAMINE, VECED_MOVE };
enum { VECRF_NONE, VECRF_HORIZONTAL, VECRF_VERTICAL, VECRF_VH, VECRF_180DEG, VECRF_SNOWFLAKE4, VECRF_END };
const bool vecrf_mirror[] = { false, true, true, true, false, true };
const char *modes[] = {"none", "horizontal", "vertical", "vertical horizontal", "180deg", "snowflake4"};
int vecedmode = VECED_REFLECT;
int vecedreflect = VECRF_NONE;

vector<Vecpt> vecs;

vector<Vecpt> getProcessedVecs() {
    if(vecedreflect == VECRF_NONE) {
        return vecs;
    } else if(vecedreflect == VECRF_HORIZONTAL) {
        vector<Vecpt> fakevec = vecs;
        for(int i = vecs.size(); i > 0; i--) {
            Vecpt gta = vecs[i-1].mirror();
            gta.y *= -1;
            gta.lhcy *= -1;
            gta.rhcy *= -1;
            fakevec.push_back(gta);
        }
        return fakevec;
    } else if(vecedreflect == VECRF_VERTICAL) {
        vector<Vecpt> fakevec = vecs;
        for(int i = vecs.size(); i > 0; i--) {
            Vecpt gta = vecs[i-1].mirror();
            gta.x *= -1;
            gta.lhcx *= -1;
            gta.rhcx *= -1;
            fakevec.push_back(gta);
        }
        return fakevec;
    } else if(vecedreflect == VECRF_VH) {
        vector<Vecpt> fakevec1 = vecs;
        for(int i = vecs.size(); i > 0; i--) {
            Vecpt gta = vecs[i-1].mirror();
            gta.y *= -1;
            gta.lhcy *= -1;
            gta.rhcy *= -1;
            fakevec1.push_back(gta);
        }
        vector<Vecpt> fakevec2 = fakevec1;
        for(int i = fakevec1.size(); i > 0; i--) {
            Vecpt gta = fakevec1[i-1].mirror();
            gta.x *= -1;
            gta.lhcx *= -1;
            gta.rhcx *= -1;
            fakevec2.push_back(gta);
        }
        return fakevec2;
    } else if(vecedreflect == VECRF_180DEG) {
        vector<Vecpt> fakevec = vecs;
        for(int i = 0; i < vecs.size(); i++) {
            Vecpt gta = vecs[i];
            gta.x *= -1;
            gta.lhcx *= -1;
            gta.rhcx *= -1;
            gta.y *= -1;
            gta.lhcy *= -1;
            gta.rhcy *= -1;
            fakevec.push_back(gta);
        }
        return fakevec;
    } else if(vecedreflect == VECRF_SNOWFLAKE4) {
        vector<Vecpt> fakevec1 = vecs;
        for(int i = vecs.size(); i > 0; i--) {
            Vecpt gta = vecs[i - 1].mirror();
            swap(gta.x, gta.y);
            swap(gta.lhcx, gta.lhcy);
            swap(gta.rhcx, gta.rhcy);
            fakevec1.push_back(gta);
        }
        vector<Vecpt> fakevec2 = fakevec1;
        for(int i = 0; i < fakevec1.size(); i++) {
            Vecpt gta = fakevec1[i];
            swap(gta.x, gta.y);
            swap(gta.lhcx, gta.lhcy);
            swap(gta.rhcx, gta.rhcy);
            gta.x *= -1;
            gta.lhcx *= -1;
            gta.rhcx *= -1;
            fakevec2.push_back(gta);
        }
        vector<Vecpt> fakevec3 = fakevec2;
        for(int i = 0; i < fakevec2.size(); i++) {
            Vecpt gta = fakevec2[i];
            gta.x *= -1;
            gta.lhcx *= -1;
            gta.rhcx *= -1;
            gta.y *= -1;
            gta.lhcy *= -1;
            gta.rhcy *= -1;
            fakevec3.push_back(gta);
        }
        return fakevec3;
    } else {
        CHECK(0);
    }
}

void saveVectors() {
    vector<Vecpt> tv = getProcessedVecs();
    dprintf("Vectors at %d\n", tv.size());
    for(int i = 0; i < tv.size(); i++) {
        for(int j = 0; j + 1 < tv.size(); j++) {
            if(tv[j].x == tv[j+1].x && tv[j].y == tv[j+1].y) {
                Vecpt nvpt = tv[j];
                nvpt.rhcurved = tv[j+1].rhcurved;
                nvpt.rhcx = tv[j+1].rhcx;
                nvpt.rhcy = tv[j+1].rhcy;
                tv[j] = nvpt;
                tv.erase(tv.begin() + j + 1);
                j--;
            }
        }
        tv.push_back(tv[0]);
        tv.erase(tv.begin());
    }
    dprintf("Vectors culled to %d\n", tv.size());
    FILE *outfile;
    {
        char timestampbuf[ 128 ];
        time_t ctmt = time(NULL);
        strftime(timestampbuf, sizeof(timestampbuf), "%Y%m%d-%H%M%S.dvec", gmtime(&ctmt));
        dprintf("%s\n", timestampbuf);
        outfile = fopen(timestampbuf, "wb");
    }
    if(!outfile) {
        dprintf("Outfile %s couldn't be opened! Didn't save!", timestampbuf);
    } else {
        for(int i = 0; i < tv.size(); i++) {
            if(tv[i].lhcurved) {
                fprintf(outfile, "(%d,%d) ", tv[i].lhcx, tv[i].lhcy);
            } else {
                fprintf(outfile, "() ");
            }
            fprintf(outfile,"%d,%d ", tv[i].x, tv[i].y);
            if(tv[i].rhcurved) {
                fprintf(outfile, "(%d,%d)\n", tv[i].rhcx, tv[i].rhcy);
            } else {
                fprintf(outfile, "()\n");
            }
        }
        fclose(outfile);
    }
}

void drawVecs(const vector<Vecpt> &vecs) {
//    dprintf("-----");
    for(int i = 0; i < vecs.size(); i++) {
//        dprintf("%d, %d, %d, %d, %d, %d, %d, %d\n", vecs[i].x, vecs[i].y, vecs[i].lhcurved, vecs[i].lhcx, vecs[i].lhcy, vecs[i].rhcurved, vecs[i].rhcx, vecs[i].rhcy);
        int next = (i + 1) % vecs.size();
        if(vecs[i].rhcurved) {
            setColor(1.0, 0.5, 0.5);
            drawCurve(
                    Float4(vecs[i].x, vecs[i].y, vecs[i].x + vecs[i].rhcx, vecs[i].y + vecs[i].rhcy),
                    Float4(vecs[next].lhcx + vecs[next].x, vecs[next].lhcy + vecs[next].y, vecs[next].x, vecs[next].y), 0.1);
        } else {
            setColor(1.0, 1.0, 1.0);
            drawLine(vecs[i].x, vecs[i].y, vecs[(i+1)%vecs.size()].x, vecs[(i+1)%vecs.size()].y, 0.1);
        }
    }
}
void drawNodeFramework(const vector<Vecpt> &vecs, int tv) {
    tv += vecs.size();
    tv %= vecs.size();
    drawBoxAround(vecs[tv].x, vecs[tv].y, 4, 0.1);
    if(vecs[tv].lhcurved) {
        drawBoxAround(vecs[tv].x + vecs[tv].lhcx, vecs[tv].y + vecs[tv].lhcy, 4, 0.1);
        drawLine(Float4(vecs[tv].x, vecs[tv].y, vecs[tv].x + vecs[tv].lhcx, vecs[tv].y + vecs[tv].lhcy), 0.1);
    }
    if(vecs[tv].rhcurved) {
        drawBoxAround(vecs[tv].x + vecs[tv].rhcx, vecs[tv].y + vecs[tv].rhcy, 4, 0.1);
        drawLine(Float4(vecs[tv].x, vecs[tv].y, vecs[tv].x + vecs[tv].rhcx, vecs[tv].y + vecs[tv].rhcy), 0.1);
    }
}

bool reversed(int mode) {
    return vecrf_mirror[mode];
}

int traverse(int start, int delta, int mode) {
    start += delta;
    while(start < 0)
        start += vecs.size() * 2;
    start %= vecs.size() * 2;
    if(start >= vecs.size()) {
        if(reversed(mode)) {
            return vecs.size() - (start - vecs.size()) - 1;
        } else {
            return start % vecs.size();
        }
    }
    return start;
}

bool mirror(int start, int delta, int mode) {
    start += delta;
    while(start < 0)
        start += vecs.size() * 2;
    start %= vecs.size() * 2;
    if(start >= vecs.size()) {
        if(reversed(mode)) {
            return true;
        } else {
            return false;
        }
    }
    return false;
}


int grid = 16;
int zoom = 200;
int activevec = 0;

int *handlex = NULL;
int *handley = NULL;

bool vecEditTick(const Controller &keys) {
    if(vecedmode == VECED_CREATE || vecedmode == VECED_MOVE) {
        if(keys.keys[2].push)
            grid *= 2;
        if(keys.keys[5].push)
            grid /= 2;
        grid = max(grid, 1);
    }
    if(vecedmode == VECED_REFLECT) {
        if(keys.l.repeat)
            vecedreflect--;
        if(keys.r.repeat)
            vecedreflect++;
        vecedreflect += VECRF_END;
        vecedreflect %= VECRF_END;
        if(keys.keys[4].repeat) {
            vecedmode = VECED_CREATE;
            Vecpt nvpt;
            nvpt.x = 0;
            nvpt.y = 0;
            nvpt.lhcurved = false;
            nvpt.rhcurved = false;
            vecs.push_back(nvpt);
        }
    } else if(vecedmode == VECED_CREATE) {
        int modif = vecs.size() - 1;
        if(keys.l.repeat)
            vecs[modif].x -= grid;
        if(keys.r.repeat)
            vecs[modif].x += grid;
        if(keys.u.repeat)
            vecs[modif].y -= grid;
        if(keys.d.repeat)
            vecs[modif].y += grid;
        if(keys.keys[0].repeat)
            vecs.push_back(Vecpt(vecs[modif]));
        if(keys.keys[4].repeat && vecs.size() > 2) {
            vecedmode = VECED_EXAMINE;
            vecs.pop_back();
        }
    } else if(vecedmode == VECED_EXAMINE) {
        if(keys.l.repeat)
            activevec--;
        if(keys.r.repeat)
            activevec++;
        if(activevec < 0)
            activevec = 0;
        if(activevec >= vecs.size())
            activevec = vecs.size() - 1;
        if(keys.keys[0].repeat && vecs[activevec].lhcurved) {
            // modify curve handle on L
            vecedmode = VECED_MOVE;
            handlex = &vecs[activevec].lhcx;
            handley = &vecs[activevec].lhcy;
        }
        if(keys.keys[2].repeat && vecs[activevec].rhcurved) {
            // modify curve handle on R
            vecedmode = VECED_MOVE;
            handlex = &vecs[activevec].rhcx;
            handley = &vecs[activevec].rhcy;
        }
        if(keys.keys[3].repeat) {
            // toggle bentness on L
            int alt = traverse(activevec, -1, vecedreflect);
            bool mirr = mirror(activevec, -1, vecedreflect);
            dprintf("%d, %d, %d, %d\n", activevec, alt, vecs.size(), mirr);
            if(mirr) {
                // these can go out of synch right now, but are easily fixable
                //CHECK(vecs[alt].lhcurved == vecs[activevec].lhcurved);
                vecs[activevec].lhcurved = !vecs[activevec].lhcurved;
                vecs[alt].lhcurved = vecs[activevec].lhcurved;
            } else {
                //CHECK(vecs[alt].rhcurved == vecs[activevec].lhcurved);
                vecs[activevec].lhcurved = !vecs[activevec].lhcurved;
                vecs[alt].rhcurved = vecs[activevec].lhcurved;
            }
        }
        if(keys.keys[5].repeat) {
            // toggle bentness on R
            int alt = traverse(activevec, 1, vecedreflect);
            bool mirr = mirror(activevec, 1, vecedreflect);
            dprintf("%d, %d, %d, %d\n", activevec, alt, vecs.size(), mirr);
            if(mirr) {
                //CHECK(vecs[alt].rhcurved == vecs[activevec].rhcurved);
                vecs[activevec].rhcurved = !vecs[activevec].rhcurved;
                vecs[alt].rhcurved = vecs[activevec].rhcurved;
            } else {
                //CHECK(vecs[alt].lhcurved == vecs[activevec].rhcurved);
                vecs[activevec].rhcurved = !vecs[activevec].rhcurved;
                vecs[alt].lhcurved = vecs[activevec].rhcurved;
            }
        }
        if(keys.keys[1].repeat) {
            // modify
            vecedmode = VECED_MOVE;
            handlex = &vecs[activevec].x;
            handley = &vecs[activevec].y;
        }
        if(keys.keys[4].repeat) {
            // add
            vecs.insert(vecs.begin() + activevec, Vecpt(vecs[activevec]));
            vecedmode = VECED_MOVE;
            handlex = &vecs[activevec].x;
            handley = &vecs[activevec].y;
        }
        if(keys.keys[6].repeat && vecs.size() > 2) {
            // delete
            vecs.erase(vecs.begin() + activevec);
            activevec = 0;
        }
        if(keys.keys[7].repeat) {
            // save
            saveVectors();
        }
    } else if(vecedmode == VECED_MOVE) {
        CHECK(handlex && handley);
        if(keys.l.repeat)
            *handlex -= grid;
        if(keys.r.repeat)
            *handlex += grid;
        if(keys.u.repeat)
            *handley -= grid;
        if(keys.d.repeat)
            *handley += grid;
        if(keys.keys[0].repeat)
            vecedmode = VECED_EXAMINE;
    } else {
        CHECK(0);
        return true;
    }
    return false;
}
void vecEditRender(void) {
    setZoom(0,0,100);
    if(vecedmode == VECED_REFLECT) {
        setColor(1.0, 1.0, 1.0);
        drawText("Reflection mode - arrows to change - k to accept", 3, 2, 2);
        drawText("Current mode", 3, 2, 6);
        drawText(modes[vecedreflect], 3, 2, 10);
    } else if(vecedmode == VECED_CREATE) {
        setColor(1.0, 1.0, 1.0);
        drawText("Create mode - arrows to move - u to add", 3, 2, 2);
        char buf[256];
        sprintf(buf, "ol to change grid - k to accept");
        drawText(buf, 3, 2, 6);
        sprintf(buf, "current grid %3d - current loc %d, %d", grid, vecs.back().x, vecs.back().y);
        drawText(buf, 3, 2, 10);
    } else if(vecedmode == VECED_EXAMINE) {
        setColor(1.0, 1.0, 1.0);
        drawText("Examine mode - arrows to traverse", 3, 2, 2);
        drawText("i move - k create - m delete", 3, 2, 6);
        drawText("uo to modify curve handle - jl straightness", 3, 2, 10);
    } else if(vecedmode == VECED_MOVE) {
        CHECK(handlex && handley);
        setColor(1.0, 1.0, 1.0);
        drawText("Move mode - arrows to move", 3, 2, 2);
        char buf[256];
        sprintf(buf, "ol to change grid - u to accept");
        drawText(buf, 3, 2, 6);
        sprintf(buf, "current grid %3d - current loc %d, %d", grid, *handlex, *handley);
        drawText(buf, 3, 2, 10);
    } else {
        CHECK(0);
    }
    setZoom(-200*1.25, -200, 200);
    setColor(0.5, 0.5, 0.5);
    if(vecedreflect == VECRF_NONE) {
    } else if(vecedreflect == VECRF_HORIZONTAL) {
        drawLine(-200*1.25, 0, 200*1.25, 0, 0.1);
    } else if(vecedreflect == VECRF_VERTICAL) {
        drawLine(0, -200, 0, 200, 0.1);
    } else if(vecedreflect == VECRF_VH) {
        drawLine(0, -200, 0, 200, 0.1);
        drawLine(-200*1.25, 0, 200*1.25, 0, 0.1);
    } else if(vecedreflect == VECRF_180DEG) {
        drawLine(-200, -200, 200, 200, 0.1);
    } else if(vecedreflect == VECRF_SNOWFLAKE4) {
        drawLine(-200, -200, 200, 200, 0.1);
        drawLine(0, -200, 0, 200, 0.1);
        drawLine(-200, 0, 200, 0, 0.1);
        drawLine(-200, 200, 200, -200, 0.1);
    } else {
        CHECK(0);
    }
    setColor(1.0, 1.0, 1.0);
    drawVecs(getProcessedVecs());
    if(vecedmode == VECED_EXAMINE || vecedmode == VECED_MOVE) {
        setColor(1.0,0.5,0.5);
        drawNodeFramework(vecs, activevec);
        setColor(1.0,1.0,1.0);
        drawNodeFramework(vecs, activevec - 1);
        drawNodeFramework(vecs, activevec + 1);
    }
}
*/
