
#include "vecedit.h"
#include "gfx.h"

#include <vector>
#include <string>
#include <stack>

using namespace std;

stack< int > modestack;

enum { VECED_EXAMINE, VECED_PATH, VECED_REFLECT, VECED_MOVE };
const char *ed_names[] = { "Examine", "Path edit", "Reflect", "Move" };
enum { VECRF_NONE, VECRF_HORIZONTAL, VECRF_VERTICAL, VECRF_VH, VECRF_180DEG, VECRF_SNOWFLAKE4, VECRF_END };
const char *rf_names[] = {"none", "horizontal", "vertical", "vertical horizontal", "180deg", "snowflake4"};

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

};

class Path {
public:
    
    float centerx;
    float centery;

    int reflect;

    vector<Vecptn> path;

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

float *write_x = NULL;
float *write_y = NULL;

int path_target = 0;
int path_curnode = 0;

int gui_vpos = 0;

void guiText(const string &in) {
    //setZoom(0, 0, 100);
    drawText(in, 2, 2, gui_vpos);
    gui_vpos += 3;
    //setZoom(-zoom*1.25, -zoom, zoom);
}

bool vecEditTick(const Controller &keys) {
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
    
    write_x = NULL;
    write_y = NULL;
    
    if(modestack.top() == VECED_EXAMINE) {
        if(keys.keys[4].repeat) {   // create path
            path_target = paths.size() - 1;
            paths.push_back(Path());
            paths[path_target].centerx = cursor_x;
            paths[path_target].centery = cursor_y;
            modestack.push(VECED_PATH);
            modestack.push(VECED_REFLECT);
        }
    } else if(modestack.top() == VECED_PATH) {
    } else if(modestack.top() == VECED_REFLECT) {
        CHECK(path_target >= 0 && path_target < paths.size());
        write_x = &paths[path_target].centerx;
        write_y = &paths[path_target].centery;
    } else {
        CHECK(0);
    }
    
    if(keys.l.repeat) cursor_x -= grid;
    if(keys.r.repeat) cursor_x += grid;
    if(keys.u.repeat) cursor_y -= grid;
    if(keys.d.repeat) cursor_y += grid;
    
    if(write_x) *write_x = cursor_x;
    if(write_y) *write_y = cursor_y;
    
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
    } else if(modestack.top() == VECED_REFLECT) {
        CHECK(path_target >= 0 && path_target < paths.size());
        guiText(StringPrintf("u/i to change reflect mode (currently %s)", rf_names[paths[path_target].reflect]));
        guiText("arrow keys to move center, / to accept");
    } else {
        CHECK(0);
    }
    
    drawLine(cursor_x - grid, cursor_y, cursor_x - grid / 2, cursor_y, 0.1);
    drawLine(cursor_x + grid, cursor_y, cursor_x + grid / 2, cursor_y, 0.1);
    drawLine(cursor_x, cursor_y - grid, cursor_x, cursor_y - grid / 2, 0.1);
    drawLine(cursor_x, cursor_y + grid, cursor_x, cursor_y + grid / 2, 0.1);
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
