
#include "interface.h"
#include "game.h"
#include "gfx.h"
#include "args.h"
#include "vecedit.h"

#include <string>
#include <vector>

using namespace std;

DEFINE_bool(vecedit, false, "vector editor mode");

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
    dprintf("keys: %d %d %d %d %d\n", keys.u.up, keys.u.down, keys.u.push, keys.u.release, keys.u.repeat);
    if(keys.f.down)
        return items[cpos].second;
    if(keys.u.repeat)
        cpos++;
    if(keys.d.repeat)
        cpos--;
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
    enum { IFM_M_NEWGAME, IFM_M_EXIT };
    int interface_mode;
    
    Game game;
    
    StdMenu mainmenu;
    
    vector< Keystates > kst;
    
public:

    bool tick(const vector< Controller > &control);
    void render() const;
    InterfaceMain();

};

bool InterfaceMain::tick(const vector< Controller > &control) {
    
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
        kst[i].f.newState(control[i].keys[0]);
    }
    
    if(interface_mode == IFM_S_MAINMENU) {
        int mrv;
        mrv = mainmenu.tick(kst[0]);
        if(mrv == IFM_M_NEWGAME) {
            game = Game();
            interface_mode = IFM_S_PLAYING;
        } else if(mrv == IFM_M_EXIT) {
            return true;
        } else {
            CHECK(mrv == -1);
        }
    } else if(interface_mode == IFM_S_PLAYING) {
        if(game.runTick(kst)) {
            interface_mode = IFM_S_MAINMENU;
        }
    } else {
        CHECK(0);
    }
    
    return false;
    
}

void InterfaceMain::render() const {
    
    if(interface_mode == IFM_S_MAINMENU) {
        mainmenu.render();
        setColor(0.5, 0.5, 0.5);
        drawText("Player one  arrow keys and uiojkl", 3, 2, 20);
        drawText("Player two  wasd       and rtyfgh", 3, 2, 24);
        drawText("Menu        arrow keys and u", 3, 2, 28);
        for(int i = 0; i < 6; i++) {
            char boof[128] = "data/a.dvec";
            boof[5] += i;
            drawVectors(loadVectors(boof),5+20*i, 50, 15, 0.1);
        }
    } else if(interface_mode == IFM_S_PLAYING) {
        game.renderToScreen(RENDERTARGET_SPECTATOR);
    } else {
        CHECK(0);
    }
    
};

InterfaceMain::InterfaceMain() {
    interface_mode = IFM_S_MAINMENU;
    mainmenu.pushMenuItem("New game", IFM_M_NEWGAME);
    mainmenu.pushMenuItem("Exit", IFM_M_EXIT);
}

InterfaceMain ifm;

Game game;

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
    
void interfaceRenderToScreen() {
    if(FLAGS_vecedit) {
        vecEditRender();
    } else {
        ifm.render();
    }
}
