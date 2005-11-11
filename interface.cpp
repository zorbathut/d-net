
#include "interface.h"
#include "metagame.h"
#include "gfx.h"
#include "args.h"
#include "vecedit.h"

#include <string>
#include <vector>

using namespace std;

DEFINE_bool(vecedit, false, "vector editor mode");
DEFINE_int(rounds_per_store, 6, "rounds between store enter");

class StdMenu {
    
    vector<pair<string, int> > items;
    int cpos;
    
public:

    void pushMenuItem(const string &name, int triggeraction);

    int tick(const Keystates &keys);
    void render() const;

    StdMenu();

};

void StdMenu::pushMenuItem(const string &name, int triggeraction) {
    items.push_back(make_pair(name, triggeraction));
}

int StdMenu::tick(const Keystates &keys) {
    if(keys.f.down)
        return items[cpos].second;
    if(keys.u.repeat)
        cpos--;
    if(keys.d.repeat)
        cpos++;
    cpos += items.size();
    cpos %= items.size();
    return -1;
}

void StdMenu::render() const {
    setZoom(0, 0, 100);
    for(int i = 0; i < items.size(); i++) {
        if(i == cpos) {
            setColor(1.0, 1.0, 1.0);
        } else {
            setColor(0.5, 0.5, 0.5);
        }
        drawText(items[i].first.c_str(), 5, 2, 2 + 6 * i);
    }
}

StdMenu::StdMenu() {
    cpos = 0;
}

class InterfaceMain {
    
    enum { IFM_S_MAINMENU, IFM_S_PLAYING };
    enum { IFM_M_NEWGAME, IFM_M_INPUTTEST, IFM_M_GRID, IFM_M_EXIT };
    int interface_mode;
    
    bool grid;
    bool inptest;
    vector<Controller> inptest_controls;
    
    Metagame game;
    
    StdMenu mainmenu;
    
    vector< Keystates > kst;
    
public:

    bool tick(const vector< Controller > &control);
    void ai(const vector<Ai *> &ais) const;
    void render() const;
    InterfaceMain();

};

bool InterfaceMain::tick(const vector< Controller > &control) {
    
    inptest_controls = control;
    
    if(kst.size() == 0) {
        CHECK(control.size() != 0);
        kst.resize(control.size());
    }
    
    CHECK(kst.size() == control.size());
    
    for(int i = 0; i < control.size(); i++) {
        CHECK(control[i].keys.size() >= 1);
        kst[i].u.newState(control[i].y > .5);
        kst[i].d.newState(control[i].y < -.5);
        kst[i].r.newState(control[i].x > .5);
        kst[i].l.newState(control[i].x < -.5);
        bool aButtonPushed = false;
        for(int j = 0; j < control[i].keys.size(); j++)
            if(control[i].keys[j].push)
                aButtonPushed = true;
        kst[i].f.newState(aButtonPushed);
    }
    
    if(interface_mode == IFM_S_MAINMENU) {
        int mrv;
        mrv = mainmenu.tick(kst[0]);
        if(mrv == IFM_M_NEWGAME) {
            game = Metagame(control.size(), FLAGS_rounds_per_store);
            interface_mode = IFM_S_PLAYING;
        } else if(mrv == IFM_M_EXIT) {
            return true;
        } else if(mrv == IFM_M_INPUTTEST) {
            inptest = !inptest;
        } else if(mrv == IFM_M_GRID) {
            grid = !grid;
        } else {
            CHECK(mrv == -1);
        }
    } else if(interface_mode == IFM_S_PLAYING) {
        if(game.runTick(control)) {
            interface_mode = IFM_S_MAINMENU;
        }
    } else {
        CHECK(0);
    }
    
    return false;
    
}

void InterfaceMain::ai(const vector<Ai *> &ai) const {
    if(interface_mode == IFM_S_MAINMENU) {
        for(int i = 0; i < ai.size(); i++)
            if(ai[i])
                ai[i]->updatePregame();
    } else if(interface_mode == IFM_S_PLAYING) {
        game.ai(ai);
    }
}

#include "dvec2.h"

void InterfaceMain::render() const {
    
    {
        Dvec2 dv2t = loadDvec2("intersecttest.dv2");
        vector<Coord2> diff[2];
        for(int i = 0; i < 2; i++) {
            for(int j = 0; j < dv2t.paths[i].vpath.size(); j++) {
                diff[i].push_back(Coord2(dv2t.paths[i].vpath[j].x + dv2t.paths[i].centerx, dv2t.paths[i].vpath[j].y + dv2t.paths[i].centery));
            }
        }
        static int mov = -1024;
        mov++;
        if(mov > 1024) mov = -1024;
        dprintf("%d\n", mov);
        for(int j = 0; j < diff[1].size(); j++) {
            diff[1][j].y += Coord(mov) / 8;
        }
        for(int i = 0; i < 2; i++) {
            bool tanksin = false;
            Coord2 ptin = getPointIn(diff[i]);
            dprintf("tanks is %d, IP is %d\n", tanksin, inPath(ptin, diff[i]));
            bool toggle = (tanksin != (inPath(ptin,  diff[i]) == -1));
            if(toggle) {
                dprintf("Toggling\n");
                reverse(diff[i].begin(),  diff[i].end());
            }
        }
        vector<vector<Coord2> > res = getDifference(diff[0], diff[1]);
        for(int i = 0; i < res.size(); i++) {
            for(int j = 0; j < res[i].size(); j++) {
                res[i][j] /= 3;
                res[i][j] += Coord2(80, 80);
            }
            drawLinePath(res[i], 0.1, true);
        }
        //CHECK(0);
    }
    
    if(interface_mode == IFM_S_MAINMENU) {
        mainmenu.render();
        setColor(0.5, 0.5, 0.5);
        drawText("Player one  arrow keys and uiojkl", 3, 2, 30);
        drawText("Player two  wasd       and rtyfgh", 3, 2, 34);
        drawText("Menu        arrow keys and u", 3, 2, 38);
        if(grid) {
            setColor(1.0, 1.0, 1.0);
            drawGrid(1, 0.01);
        }
        if(inptest) {
            setZoom(0, 0, 600);
            setColor(1.0, 1.0, 1.0);
            const float xsiz = 100;
            const float bord = xsiz / 25;
            const float usablesiz = xsiz - bord * 4;
            const float crosshair = usablesiz / 2;
            const float ysiz = crosshair + bord * 2;
            const float crosshairc = crosshair / 2;
            const float textsize = crosshair / 4;
            const float textrsize = textsize * 0.8;
            const int wid = int(800 / xsiz);
            const int textymax = int(crosshair / textsize);
            const int textxmax = 2;
            const float textxofs = crosshair / textxmax;
            for(int i = 0; i < inptest_controls.size() * 4; i++) {
                const Controller &ct = inptest_controls[i % inptest_controls.size()];
                float x = (i % wid) * xsiz;
                float y = (i / wid) * ysiz + 300;
                Float4 chbox(x + bord, y + bord, x + bord + crosshair, y + bord + crosshair);
                drawLine(Float4(chbox.sx, chbox.sy, chbox.sx + crosshair / 4, chbox.sy), 0.1);
                drawLine(Float4(chbox.sx, chbox.sy, chbox.sx, chbox.sy + crosshair / 4), 0.1);
                drawLine(Float4(chbox.sx, chbox.ey, chbox.sx + crosshair / 4, chbox.ey), 0.1);
                drawLine(Float4(chbox.sx, chbox.ey, chbox.sx, chbox.ey - crosshair / 4), 0.1);
                drawLine(Float4(chbox.ex, chbox.sy, chbox.ex - crosshair / 4, chbox.sy), 0.1);
                drawLine(Float4(chbox.ex, chbox.sy, chbox.ex, chbox.sy + crosshair / 4), 0.1);
                drawLine(Float4(chbox.ex, chbox.ey, chbox.ex - crosshair / 4, chbox.ey), 0.1);
                drawLine(Float4(chbox.ex, chbox.ey, chbox.ex, chbox.ey - crosshair / 4), 0.1);
                drawCrosshair(ct.x * crosshairc + bord + crosshairc + x, -ct.y * crosshairc + bord + crosshairc + y, crosshair / 4, 0.1);
                float textx = x + bord * 3 + crosshair;
                float texty = y + bord;
                int ctxt = 0;
                int kd = 0;
                for(int j = 0; j < ct.keys.size(); j++)
                    if(ct.keys[j].down)
                        kd++;
                for(int j = 0; j < ct.keys.size(); j++) {
                    if(ct.keys[j].down) {
                        string tbd;
                        if(ctxt >= textymax * textxmax) {
                            continue;
                        } else if(ctxt == textymax * textxmax - 1 && kd > textymax * textxmax) {
                            tbd = "...";
                        } else {
                            tbd = StringPrintf("%d", j);
                        }
                        if(tbd != "") {
                            drawText(tbd, textrsize, textx + ctxt / textymax * textxofs, texty + (ctxt % textymax) * textsize);
                            ctxt++;
                        }
                    }
                }
            }
        }
    } else if(interface_mode == IFM_S_PLAYING) {
        game.renderToScreen();
    } else {
        CHECK(0);
    }
    
};

InterfaceMain::InterfaceMain() {
    interface_mode = IFM_S_MAINMENU;
    mainmenu.pushMenuItem("New game", IFM_M_NEWGAME);
    mainmenu.pushMenuItem("Input test", IFM_M_INPUTTEST);
    mainmenu.pushMenuItem("Grid toggle", IFM_M_GRID);
    mainmenu.pushMenuItem("Exit", IFM_M_EXIT);
    grid = false;
    inptest = false;
}

InterfaceMain ifm;

void interfaceInit() {
    ifm = InterfaceMain();
}

bool interfaceRunTick( const vector< Controller > &control ) {
    if(FLAGS_vecedit) {
        return vecEditTick(control[0]);
    } else {
        return ifm.tick(control);
    }
}

void interfaceRunAi(const vector<Ai *> &ais) {
    if(FLAGS_vecedit) {
        for(int i = 0; i < ais.size(); i++)
            CHECK(!ais[i]);
    } else {
        return ifm.ai(ais);
    }
}
    
void interfaceRenderToScreen() {
    if(FLAGS_vecedit) {
        vecEditRender();
    } else {
        ifm.render();
    }
}
