
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
    enum { IFM_M_NEWGAME, IFM_M_GRID, IFM_M_EXIT };
    int interface_mode;
    
    bool gridinate;
    
    Metagame game;
    
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
        } else if(mrv == IFM_M_GRID) {
            gridinate = !gridinate;
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

void InterfaceMain::render() const {
    
    if(interface_mode == IFM_S_MAINMENU) {
        mainmenu.render();
        setColor(0.5, 0.5, 0.5);
        drawText("Player one  arrow keys and uiojkl", 3, 2, 30);
        drawText("Player two  wasd       and rtyfgh", 3, 2, 34);
        drawText("Menu        arrow keys and u", 3, 2, 38);
        if(gridinate) {
            setColor(1.0, 1.0, 1.0);
            drawGrid(1, 0.01);
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
    mainmenu.pushMenuItem("Grid toggle", IFM_M_GRID);
    mainmenu.pushMenuItem("Exit", IFM_M_EXIT);
    gridinate = false;
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
    
void interfaceRenderToScreen() {
    if(FLAGS_vecedit) {
        vecEditRender();
    } else {
        ifm.render();
    }
}
