
#include "vecedit.h"
#include "gfx.h"

#include <vector>
#include <string>

using namespace std;

enum { VECED_REFLECT, VECED_CREATE, VECED_EXAMINE, VECED_MOVE };
enum { VECRF_NONE, VECRF_HORIZONTAL, VECRF_VERTICAL, VECRF_END };
const char *modes[] = {"none", "horizontal", "vertical"};
int vecedmode = VECED_REFLECT;
int vecedreflect = VECRF_NONE;

class Vecpt {
public:
    int x;
    int y;
    int lhcx;
    int lhcy;
    int rhcx;
    int rhcy;
    bool lhcurved;
    bool rhcurved;

    Vecpt mirror() const {
        Vecpt gn = *this;
        swap(gn.lhcx, gn.rhcx);
        swap(gn.lhcy, gn.rhcy);
        swap(gn.lhcurved, gn.rhcurved);
        return gn;
    }
    
};

vector<Vecpt> vecs;

void drawVecs(const vector<Vecpt> &vecs) {
    for(int i = 0; i < vecs.size(); i++) {
        int next = (i + 1) % vecs.size();
        if(vecs[i].rhcurved) {
            drawCurve(Float4(vecs[i].x, vecs[i].y, vecs[i].rhcx, vecs[i].rhcy), Float4(vecs[next].lhcx, vecs[next].lhcy, vecs[next].x, vecs[next].y), 0.1);
        } else {
            drawLine(vecs[i].x, vecs[i].y, vecs[(i+1)%vecs.size()].x, vecs[(i+1)%vecs.size()].y, 0.1);
        }
    }
}
void drawNodeFramework(const vector<Vecpt> &vecs, int tv) {
    tv += vecs.size();
    tv %= vecs.size();
    drawBoxAround(vecs[tv].x, vecs[tv].y, 4, 0.1);
    if(vecs[tv].lhcurved) {
        drawBoxAround(vecs[tv].lhcx, vecs[tv].lhcy, 4, 0.1);
        drawLine(Float4(vecs[tv].x, vecs[tv].y, vecs[tv].lhcx, vecs[tv].lhcy), 0.1);
    }
    if(vecs[tv].rhcurved) {
        drawBoxAround(vecs[tv].rhcx, vecs[tv].rhcy, 4, 0.1);
        drawLine(Float4(vecs[tv].x, vecs[tv].y, vecs[tv].rhcx, vecs[tv].rhcy), 0.1);
    }
}

bool reversed(int mode) {
    return mode == VECRF_HORIZONTAL || mode == VECRF_VERTICAL;
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
            if(mirr) {
                CHECK(vecs[alt].lhcurved == vecs[activevec].lhcurved);
                vecs[activevec].lhcurved = !vecs[activevec].lhcurved;
                vecs[alt].lhcurved = vecs[activevec].lhcurved;
                if(vecs[activevec].lhcurved) {
                    vecs[activevec].lhcx = vecs[activevec].x + 4;
                    vecs[activevec].lhcy = vecs[activevec].y + 4;
                    vecs[alt].lhcx = vecs[alt].x + 4;
                    vecs[alt].lhcy = vecs[alt].y + 4;
                } else {
                    vecs[activevec].lhcx = vecs[activevec].x;
                    vecs[activevec].lhcy = vecs[activevec].y;
                    vecs[alt].lhcx = vecs[alt].x;
                    vecs[alt].lhcy = vecs[alt].y;
                }
            } else {
                CHECK(vecs[alt].rhcurved == vecs[activevec].lhcurved);
                vecs[activevec].lhcurved = !vecs[activevec].lhcurved;
                vecs[alt].rhcurved = vecs[activevec].lhcurved;
                if(vecs[activevec].lhcurved) {
                    vecs[activevec].lhcx = vecs[activevec].x + 4;
                    vecs[activevec].lhcy = vecs[activevec].y + 4;
                    vecs[alt].rhcx = vecs[alt].x + 4;
                    vecs[alt].rhcy = vecs[alt].y + 4;
                } else {
                    vecs[activevec].lhcx = vecs[activevec].x;
                    vecs[activevec].lhcy = vecs[activevec].y;
                    vecs[alt].rhcx = vecs[alt].x;
                    vecs[alt].rhcy = vecs[alt].y;
                }
            }
        }
        if(keys.keys[5].repeat) {
            // toggle bentness on R
            int alt = traverse(activevec, 1, vecedreflect);
            bool mirr = mirror(activevec, 1, vecedreflect);
            if(mirr) {
                CHECK(vecs[alt].rhcurved == vecs[activevec].rhcurved);
                vecs[activevec].rhcurved = !vecs[activevec].rhcurved;
                vecs[alt].rhcurved = vecs[activevec].rhcurved;
                if(vecs[activevec].rhcurved) {
                    vecs[activevec].rhcx = vecs[activevec].x + 4;
                    vecs[activevec].rhcy = vecs[activevec].y + 4;
                    vecs[alt].rhcx = vecs[alt].x + 4;
                    vecs[alt].rhcy = vecs[alt].y + 4;
                } else {
                    vecs[activevec].rhcx = vecs[activevec].x;
                    vecs[activevec].rhcy = vecs[activevec].y;
                    vecs[alt].rhcx = vecs[alt].x;
                    vecs[alt].rhcy = vecs[alt].y;
                }
            } else {
                CHECK(vecs[alt].lhcurved == vecs[activevec].rhcurved);
                vecs[activevec].rhcurved = !vecs[activevec].rhcurved;
                vecs[alt].lhcurved = vecs[activevec].rhcurved;
                if(vecs[activevec].rhcurved) {
                    vecs[activevec].rhcx = vecs[activevec].x + 4;
                    vecs[activevec].rhcy = vecs[activevec].y + 4;
                    vecs[alt].lhcx = vecs[alt].x + 4;
                    vecs[alt].lhcy = vecs[alt].y + 4;
                } else {
                    vecs[activevec].rhcx = vecs[activevec].x;
                    vecs[activevec].rhcy = vecs[activevec].y;
                    vecs[alt].lhcx = vecs[alt].x;
                    vecs[alt].lhcy = vecs[alt].y;
                }
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
    if(vecedreflect == VECRF_NONE) {
    } else if(vecedreflect == VECRF_HORIZONTAL) {
        setColor(0.5, 0.5, 0.5);
        drawLine(-200*1.25, 0, 200*1.25, 0, 0.1);
    } else if(vecedreflect == VECRF_VERTICAL) {
        setColor(0.5, 0.5, 0.5);
        drawLine(0, -200, 0, 200, 0.1);
    } else {
        CHECK(0);
    }
    if(vecedreflect == VECRF_NONE) {
        drawVecs(vecs);
    } else if(vecedreflect == VECRF_HORIZONTAL) {
        vector<Vecpt> fakevec = vecs;
        for(int i = vecs.size(); i > 0; i--) {
            Vecpt gta = vecs[i-1].mirror();
            gta.y *= -1;
            gta.lhcy *= -1;
            gta.rhcy *= -1;
            fakevec.push_back(gta);
        }
        setColor(1.0, 1.0, 1.0);
        drawVecs(fakevec);
    } else if(vecedreflect == VECRF_VERTICAL) {
                vector<Vecpt> fakevec = vecs;
        for(int i = vecs.size(); i > 0; i--) {
            Vecpt gta = vecs[i-1].mirror();
            gta.x *= -1;
            gta.lhcx *= -1;
            gta.rhcx*= -1;
            fakevec.push_back(gta);
        }
        drawVecs(fakevec);
    } else {
        CHECK(0);
    }
    if(vecedmode == VECED_EXAMINE || vecedmode == VECED_MOVE) {
        setColor(1.0,0.5,0.5);
        drawNodeFramework(vecs, activevec);
        setColor(1.0,1.0,1.0);
        drawNodeFramework(vecs, activevec - 1);
        drawNodeFramework(vecs, activevec + 1);
    }
}
